/* Genesis Terminal UI — scrollback, keyboard entry, menu dispatch. */
#include "serial_terminal.h"
#include "../common/mixnet_line.h"
#include "../include/mixnet_proto.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Layout constants                                                  */
/* ------------------------------------------------------------------ */
#define ROW_STATUS   0
#define ROW_SB_FIRST 1
#define ROW_SB_LAST  21
#define ROW_INPUT    22
#define ROW_MENU1    23
#define ROW_MENU2    24

#define COLS 40

/* ------------------------------------------------------------------ */
/*  Keyboard grid (10 columns, 4 rows, paged 2 rows at a time)        */
/* ------------------------------------------------------------------ */
#define KB_COLS 10
#define KB_ROWS 4
#define KB_ROWS_VIS 2
#define KB_PAGES 2

static const char kb_grid[KB_ROWS][KB_COLS] = {
	{'A','B','C','D','E','F','G','H','I','J'},
	{'K','L','M','N','O','P','Q','R','S','T'},
	{'U','V','W','X','Y','Z','0','1','2','3'},
	{'4','5','6','7','8','9','.','!','?',' '}
};

/* ------------------------------------------------------------------ */
/*  Menu items                                                        */
/* ------------------------------------------------------------------ */
#define MENU_COUNT 6
static const char *menu_labels[MENU_COUNT] = {
	"Help", "Nick", "Join", "Rooms", "Who", "Msg"
};

/* Which protocol action each menu item triggers when input is confirmed */
typedef enum {
	ACT_NONE,
	ACT_HELLO,   /* send HELLO <nick> */
	ACT_JOIN,    /* send JOIN <room> */
	ACT_MSG,     /* send MSG <text> */
	ACT_RAW      /* send input line as-is */
} TermAction;

/* ------------------------------------------------------------------ */
/*  State                                                             */
/* ------------------------------------------------------------------ */
static term_scrollback_t sb;
static term_input_t in;
static char prev_hist[4][TERM_LINE_MAX];
static u16 hist_count;

static TermMode mode;
static TermAction pending_action; /* action to take when input confirmed */
static u8 kb_page;                /* 0 or 1 */
static u8 kb_x, kb_y;            /* cursor within visible rows */
static u16 col_bright;           /* palette cycle counter for cursor */

/* ------------------------------------------------------------------ */
/*  Help text (show when user asks for help)                          */
/* ------------------------------------------------------------------ */
static const char *help_lines[] = {
	"=== mixnet Genesis Terminal ===",
	" Connect MD Port 2 serial to PC",
	" bridge (4800 bps), then TCP to",
	" mixnetd server on port 19677.",
	"",
	" [A] Help    [B] Nick   [C] Join",
	" [LEFT] Msg  [RIGHT] Rooms [DOWN] Who",
	" [UP] scroll back  [START] keyboard",
	" Keyboard: [A] select  [B] backspace",
	" [C] cancel  [START] send  [UP/DN] scroll keyboard",
	"",
	" Commands: :h  :nick <n>  :g <room>",
	" :rooms  :who  :m <nick> <msg>  :q",
	" Type a command directly via START->keyboard, or",
	" use the menu buttons above.",
	"",
	" Press any button to close."
};

static u8 help_visible;

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                  */
/* ------------------------------------------------------------------ */
static void clear_row(u8 row)
{
	char buf[COLS + 1];
	memset(buf, ' ', COLS);
	buf[COLS] = '\0';
	VDP_drawText(buf, 0, row);
}

static void ins_char(char c)
{
	if (in.pos < TERM_INPUT_MAX - 2) {
		/* shift existing text right by 1 */
		for (int i = in.pos; i > 0; i--)
			in.buf[i] = in.buf[i - 1];
		in.buf[0] = c;
		in.pos++;
	}
}
static void del_char(void)
{
	if (in.pos > 0) {
		for (int i = 0; i < in.pos - 1; i++)
			in.buf[i] = in.buf[i + 1];
		in.pos--;
	}
}
static void clear_input(void)
{
	in.pos = 0;
	in.buf[0] = '\0';
}

/* Manual string building (no snprintf in SGDK libc).
 * Returns length written (excluding NUL). */
static int append_str(char *dst, int cap, const char *src)
{
	int len = 0;
	while (len < cap - 1 && src[len]) {
		dst[len] = src[len];
		len++;
	}
	dst[len] = '\0';
	return len;
}

/* ------------------------------------------------------------------ */
/*  Timestamp                                                         */
/* ------------------------------------------------------------------ */
static void timestamp(char *out, int cap)
{
	u16 v = vtimer;
	u16 minutes = (v / 3600) % 60;
	u16 seconds = (v / 60) % 60;
	sprintf(out, "%02u:%02u", minutes, seconds);
	(void)cap;
}

/* ------------------------------------------------------------------ */
/*  Scrollback helpers                                                */
/* ------------------------------------------------------------------ */
static void sb_push(const char *line)
{
	u16 next = (sb.head + 1) % TERM_SCROLLBACK_LINES;
	strncpy(sb.lines[sb.head], line, TERM_LINE_MAX - 1);
	sb.lines[sb.head][TERM_LINE_MAX - 1] = '\0';
	sb.head = next;
	/* Keep viewport at most recent */
	sb.viewport = 0;
}

static void sb_scroll(s16 delta)
{
	s16 max = (s16)TERM_SCROLLBACK_LINES - 1;
	sb.viewport += delta;
	if (sb.viewport > 0) sb.viewport = 0;
	if (sb.viewport < -max) sb.viewport = -max;
}

/* ------------------------------------------------------------------ */
/*  Send a line via serial (called from main)                         */
/* ------------------------------------------------------------------ */
static char tx_buf[TERM_INPUT_MAX + 16];
static bool tx_pending;

void terminal_push_line(const char *line)
{
	char ts[8];
	char prefixed[TERM_LINE_MAX];
	timestamp(ts, sizeof ts);
	append_str(prefixed, sizeof prefixed, "[");
	append_str(prefixed, sizeof prefixed, ts);
	append_str(prefixed, sizeof prefixed, "] ");
	append_str(prefixed, sizeof prefixed, line);
	sb_push(prefixed);
}

int terminal_poll_tx(char *buf, int cap)
{
	if (!tx_pending) return 0;
	tx_pending = 0;
	strncpy(buf, tx_buf, cap - 1);
	buf[cap - 1] = '\0';
	return 1;
}

static void queue_tx(const char *line)
{
	strncpy(tx_buf, line, sizeof tx_buf - 2);
	int n = strlen(tx_buf);
	if (n > 0 && tx_buf[n - 1] != '\n') {
		tx_buf[n] = '\n';
		tx_buf[n + 1] = '\0';
	}
	tx_pending = 1;
}

/* ------------------------------------------------------------------ */
/*  Confirm current input line                                        */
/* ------------------------------------------------------------------ */
static void confirm_input(void)
{
	char line[TERM_INPUT_MAX + 16];
	const char *cmd = in.buf;

	switch (pending_action) {
	case ACT_HELLO:
	case ACT_JOIN:
	case ACT_MSG: {
		const char *verb = (pending_action == ACT_HELLO) ? MX_HELLO :
		                   (pending_action == ACT_JOIN) ? MX_JOIN : MX_MSG;
		append_str(line, sizeof line, verb);
		append_str(line, sizeof line, " ");
		append_str(line, sizeof line, cmd);
		queue_tx(line);
		line[0] = '\0';
		append_str(line, sizeof line, ">>> ");
		append_str(line, sizeof line, verb);
		append_str(line, sizeof line, " ");
		append_str(line, sizeof line, cmd);
		sb_push(line);
		break;
	}
	default:
	case ACT_RAW:
		queue_tx(in.buf);
		sb_push(in.buf);
		break;
	}

	/* Save to history */
	if (in.pos > 0) {
		u16 hi = hist_count % 4;
		strncpy(prev_hist[hi], in.buf, TERM_LINE_MAX - 1);
		hist_count++;
	}
	clear_input();
	pending_action = ACT_NONE;
}

/* ------------------------------------------------------------------ */
/*  Initialisation                                                    */
/* ------------------------------------------------------------------ */
void terminal_init(void)
{
	memset(&sb, 0, sizeof sb);
	clear_input();
	memset(prev_hist, 0, sizeof prev_hist);
	hist_count = 0;
	mode = TERM_MODE_NORMAL;
	pending_action = ACT_NONE;
	kb_page = 0;
	kb_x = 0;
	kb_y = 0;
	help_visible = 0;
	tx_pending = 0;
	col_bright = 0;

	VDP_clearTextArea(0, 0, COLS, 28);
}

/* ------------------------------------------------------------------ */
/*  Render                                                            */
/* ------------------------------------------------------------------ */
void terminal_render(void)
{
	char buf[COLS + 1];

	col_bright++;

	if (help_visible) {
		/* Show help overlay */
		VDP_clearTextArea(0, 0, COLS, 25);
		int total = sizeof help_lines / sizeof help_lines[0];
		int rows = total < 22 ? total : 22;
		for (int r = 0; r < rows; r++) {
			VDP_drawText(help_lines[r], 0, r);
		}
		return;
	}

	/* --- Row 0: Status bar --- */
	append_str(buf, sizeof buf, "\x81 mixnet 4800 bps ");
	if (mode == TERM_MODE_KEYBOARD) append_str(buf, sizeof buf, "[KB]");
	append_str(buf, sizeof buf, "\x80");
	VDP_drawText(buf, 0, ROW_STATUS);

	/* --- Rows 1-21: Scrollback --- */
	u16 idx;
	s16 offset = sb.viewport;
	for (int r = ROW_SB_FIRST; r <= ROW_SB_LAST; r++) {
		clear_row(r);
		/* Calculate which scrollback line to show */
		s16 line_idx = (s16)sb.head + offset + (r - ROW_SB_FIRST);
		while (line_idx < 0) line_idx += TERM_SCROLLBACK_LINES;
		while (line_idx >= (s16)TERM_SCROLLBACK_LINES) line_idx -= TERM_SCROLLBACK_LINES;
		idx = (u16)line_idx;
		if (sb.lines[idx][0] != '\0') {
			strncpy(buf, sb.lines[idx], COLS);
			buf[COLS] = '\0';
			VDP_drawText(buf, 0, r);
		}
	}

	/* --- Row 22: Input line --- */
	clear_row(ROW_INPUT);
	append_str(buf, sizeof buf, "> ");
	append_str(buf, sizeof buf, in.buf);
	if ((col_bright & 15) < 8 && mode == TERM_MODE_KEYBOARD) {
		/* Blink cursor */
		int len = strlen(buf);
		if (len < COLS) { buf[len] = '_'; buf[len + 1] = '\0'; }
	}
	VDP_drawText(buf, 0, ROW_INPUT);

	if (mode == TERM_MODE_NORMAL) {
		/* --- Rows 23-24: Menu bar --- */
		clear_row(ROW_MENU1);
		clear_row(ROW_MENU2);
		append_str(buf, sizeof buf, "[A:");
		append_str(buf, sizeof buf, menu_labels[0]);
		append_str(buf, sizeof buf, "] [B:");
		append_str(buf, sizeof buf, menu_labels[1]);
		append_str(buf, sizeof buf, "] [C:");
		append_str(buf, sizeof buf, menu_labels[2]);
		append_str(buf, sizeof buf, "]");
		VDP_drawText(buf, 0, ROW_MENU1);
		buf[0] = '\0';
		append_str(buf, sizeof buf, "[LEFT:");
		append_str(buf, sizeof buf, menu_labels[5]);
		append_str(buf, sizeof buf, "] [RIGHT:");
		append_str(buf, sizeof buf, menu_labels[3]);
		append_str(buf, sizeof buf, "] [DOWN:");
		append_str(buf, sizeof buf, menu_labels[4]);
		append_str(buf, sizeof buf, "]");
		VDP_drawText(buf, 0, ROW_MENU2);
	} else {
		/* --- Keyboard grid on rows 23-24 --- */
		u8 page_row = kb_page * KB_ROWS_VIS;
		for (int r = 0; r < KB_ROWS_VIS; r++) {
			clear_row(ROW_MENU1 + r);
			for (int c = 0; c < KB_COLS; c++) {
				char ch = kb_grid[page_row + r][c];
				int scr_row = ROW_MENU1 + r;
				int scr_col = c * 4; /* 3 spaces + char */
				/* Highlight cursor position */
				u8 bright = (kb_y == (u8)r && kb_x == (u8)c) ? (col_bright & 8 ? 1 : 0) : 0;
				if (bright) {
					char tmp[2] = { ch, '\0' };
					VDP_setTextPalette(PAL1);
					VDP_drawText(tmp, scr_col, scr_row);
					VDP_setTextPalette(PAL0);
				} else {
					char tmp[2] = { ch, '\0' };
					VDP_drawText(tmp, scr_col, scr_row);
				}
			}
		}
	}
}

/* ------------------------------------------------------------------ */
/*  Joypad handler                                                    */
/* ------------------------------------------------------------------ */
void terminal_on_joy(u16 joy, u16 changed, u16 state)
{
	if (joy != JOY_1) return;

	if (help_visible) {
		if (changed & state) help_visible = 0;
		return;
	}

	if (mode == TERM_MODE_NORMAL) {
		/* Menu dispatch */
		if (changed & state) {
			if (state & BUTTON_A) {
				/* Help */
				help_visible = 1;
				return;
			}
			if (state & BUTTON_B) {
				mode = TERM_MODE_KEYBOARD;
				pending_action = ACT_HELLO;
				clear_input();
				kb_page = 0; kb_x = 0; kb_y = 0;
				return;
			}
			if (state & BUTTON_C) {
				mode = TERM_MODE_KEYBOARD;
				pending_action = ACT_JOIN;
				clear_input();
				kb_page = 0; kb_x = 0; kb_y = 0;
				return;
			}
			if (state & BUTTON_LEFT) {
				mode = TERM_MODE_KEYBOARD;
				pending_action = ACT_MSG;
				clear_input();
				kb_page = 0; kb_x = 0; kb_y = 0;
				return;
			}
			if (state & BUTTON_RIGHT) {
				queue_tx(":rooms\n");
				sb_push(">>> :rooms");
				return;
			}
			if (state & BUTTON_DOWN) {
				queue_tx(":who\n");
				sb_push(">>> :who");
				return;
			}
			if (state & BUTTON_UP) {
				sb_scroll(1);
				return;
			}
			if (state & BUTTON_START) {
				mode = TERM_MODE_KEYBOARD;
				pending_action = ACT_RAW;
				clear_input();
				kb_page = 0; kb_x = 0; kb_y = 0;
				return;
			}
		}
	} else {
		/* TERM_MODE_KEYBOARD */
		if (changed & state) {
			if (state & BUTTON_A) {
				/* Select character */
				u8 row = kb_page * KB_ROWS_VIS + kb_y;
				char c = kb_grid[row][kb_x];
				ins_char(c);
				return;
			}
			if (state & BUTTON_B) {
				del_char();
				return;
			}
			if (state & BUTTON_C) {
				mode = TERM_MODE_NORMAL;
				if (pending_action != ACT_RAW) clear_input();
				pending_action = ACT_NONE;
				return;
			}
			if (state & BUTTON_START) {
				if (in.pos > 0) {
					confirm_input();
				}
				mode = TERM_MODE_NORMAL;
				pending_action = ACT_NONE;
				return;
			}
			if (state & BUTTON_UP) {
				if (kb_y > 0) kb_y--;
				else if (kb_page > 0) { kb_page--; kb_y = KB_ROWS_VIS - 1; }
				return;
			}
			if (state & BUTTON_DOWN) {
				if (kb_y < KB_ROWS_VIS - 1) kb_y++;
				else if (kb_page < KB_PAGES - 1) { kb_page++; kb_y = 0; }
				return;
			}
			if (state & BUTTON_LEFT) {
				if (kb_x > 0) kb_x--;
				else kb_x = KB_COLS - 1;
				return;
			}
			if (state & BUTTON_RIGHT) {
				if (kb_x < KB_COLS - 1) kb_x++;
				else kb_x = 0;
				return;
			}
		}
	}
}
