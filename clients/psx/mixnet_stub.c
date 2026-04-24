/*
 * 68mixCross — PlayStation: Mixnet Navigator (Netscape-style hub for mixnetd).
 * Serial (SIO1) + PC bridge: PS1 line-framed bytes <-> host TCP to mixnetd.
 * Build:  mixnet_navigator.c + mixnet_stub.c  (+ common/mixnet_line in navigator TU)
 */
#include "mixnet_psx.h"
#include "mixnet_navigator.h"
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"
#include "../common/mixnet_line.h"
#include "../common/mixnet_line.c"

#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libsio.h>
#include <string.h>

typedef struct {
	DRAWENV draw;
	DISPENV disp;
} mixnet_db_t;

static mixnet_db_t s_db[2];
static int s_flip;
static int s_vinit;
static int s_sio_ok;
static char s_line[52];
static unsigned long s_prev_pad;
static int s_cool;

static mixnet_line_rx_t s_line_rx;
static char s_view[4096];

/* ---- in-memory only (line layer self-test; no hardware) ---- */
#define PSX_TX_CAP 64
static unsigned char s_test_tx[PSX_TX_CAP];
static unsigned s_test_len;

static void selftest_link_tx(void* u, int byte) {
	(void)u;
	if (s_test_len < (unsigned)PSX_TX_CAP - 1u) s_test_tx[s_test_len++] = (unsigned char)byte;
}

/* SIO TX: mixnetd-bound lines (may spin briefly if cable disconnected) */
#define SIO_TX_SPIN 800000u

static int sio_putb(int c) {
	unsigned w;
	if (!s_sio_ok) return -1;
	for (w = 0u; w < SIO_TX_SPIN; w++) {
		_sio_control(2, 1, 0);
		if ((unsigned long)_sio_control(0, 0, 0) & (unsigned long)SR_TXRDY) {
			_sio_control(1, 4, c & 0xff);
			return 0;
		}
	}
	return -1;
}

static void mixnet_sio_init(void) {
	/* 8N1, 115200 — match PC bridge / emulators. Polling (no Sio1Callback / no RXIEN). */
	_sio_control(1, 1, (unsigned long)(CR_RXEN | CR_TXEN | CR_RTS | CR_DTR));
	_sio_control(1, 2, (unsigned long)(MR_SB_11 | MR_CHLEN_8 | MR_BR_16));
	_sio_control(1, 3, 115200u);
	s_sio_ok = 1;
}

static void mixnet_psx_link_tx(void* u, int byte) {
	(void)u;
	(void)sio_putb(byte);
}

/* Serial RX = one line at a time from mixnetd (bridge relays TCP to SIO). */
static void sio_pump_incoming(void) {
	char lbuf[MIXNET_MAX_LINE + 4];
	if (!s_sio_ok) return;
	_sio_control(2, 1, 0);
	while (((unsigned long)_sio_control(0, 0, 0) & (unsigned long)SR_RXRDY) != 0) {
		int c = (int)(unsigned char)_sio_control(0, 4, 0);
		if (mixnet_line_rx_byte(&s_line_rx, c, lbuf, (size_t)sizeof lbuf))
			(void)mixnet_nav_on_incoming_line(lbuf);
	}
}

static void fnt_print_lines(const char* text, int n_bytes) {
	const char* end;
	int n, i, c;
	if (!text) return;
	if (n_bytes > 4090) n_bytes = 4090;
	end = text + n_bytes;
	while (text < end && *text) {
		n = 0;
		for (i = 0; i < 48 && (text + i) < end && text[i] && text[i] != '\n' && text[i] != '\r';
		     i++) {
			c = (unsigned char)text[i];
			if (c < 32 && c != '\t') c = ' ';
			s_line[n++] = (char)c;
		}
		s_line[n] = '\0';
		FntPrint("%s\n", s_line);
		text += i;
		while (text < end && (*text == '\n' || *text == '\r')) text++;
	}
}

void mixnet_psx_init_video(void) {
	if (s_vinit) return;
	ResetCallback();
	mixnet_sio_init();
	PadInit(0);
	ResetGraph(0);
	SetGraphDebug(0);
	SetDefDrawEnv(&s_db[0].draw, 0, 0, 320, 240);
	SetDefDrawEnv(&s_db[1].draw, 0, 240, 320, 240);
	SetDefDispEnv(&s_db[0].disp, 0, 240, 320, 240);
	SetDefDispEnv(&s_db[1].disp, 0, 0, 320, 240);
	s_db[0].draw.isbg = s_db[1].draw.isbg = 1;
	FntLoad(960, 256);
	SetDumpFnt(FntOpen(4, 4, 312, 232, 0, 2000));
	SetDispMask(1);
	s_flip = 0;
	s_vinit = 1;
}

void mixnet_psx_pump(void) { sio_pump_incoming(); }

void mixnet_psx_blt_screen(const char* text, int n_bytes) {
	mixnet_db_t* cdb;
	if (!s_vinit) return;
	if (n_bytes < 0) n_bytes = 0;
	s_flip = s_flip ? 0 : 1;
	cdb = &s_db[s_flip];
	DrawSync(0);
	VSync(0);
	PutDispEnv(&cdb->disp);
	PutDrawEnv(&cdb->draw);
	fnt_print_lines(text, n_bytes);
	FntFlush(-1);
}

/* ---- line layer smoke test (RAM only) ---- */
static int line_layer_selftest(void) {
	mixnet_line_rx_t rx;
	char out[MIXNET_MAX_LINE + 4];
	int i;
	const char* sample = "INFO line-test\r\n";
	s_test_len = 0u;
	if (mixnet_write_line("JOIN x", selftest_link_tx, NULL) != 0) return 0;
	if (s_test_len < 4u) return 0;
	mixnet_line_rx_init(&rx);
	for (i = 0; sample[i]; i++) {
		if (mixnet_line_rx_byte(&rx, (int)(unsigned char)sample[i], out, (size_t)sizeof out)) {
			if (strcmp(out, "INFO line-test") == 0) return 1;
		}
	}
	return 0;
}

/* Pad: edge-triggered, short cool-down to limit repeats */
static void handle_pad(void) {
	unsigned long p, edge;
	if (s_cool > 0) {
		s_cool--;
		return;
	}
	p = PadRead(0);
	edge = p & ~s_prev_pad;
	s_prev_pad = p;
	if (edge & PADLup) (void)mixnet_nav_user_key("1", NULL, 0);
	if (edge & PADLdown) (void)mixnet_nav_user_key("2", NULL, 0);
	if (edge & PADLright) (void)mixnet_nav_user_key("3", NULL, 0);
	if (edge & PADLleft) (void)mixnet_nav_user_key("4", NULL, 0);
	if (edge & PADL1) (void)mixnet_nav_user_key("5", NULL, 0);
	if (edge & PADL2) (void)mixnet_nav_user_key("6", NULL, 0);
	if (edge & PADselect) (void)mixnet_nav_user_key(":h", NULL, 0);
	if (edge & PADstart) (void)mixnet_nav_user_key(":g", NULL, 0);
	if (edge & PADR1) (void)mixnet_nav_user_key("PING", NULL, 0);
	if (edge) s_cool = 12;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	(void)MX_HELLO[0];
	if (!line_layer_selftest()) return 1;
	mixnet_psx_init_video();
	mixnet_nav_init(mixnet_psx_link_tx, NULL);
	mixnet_line_rx_init(&s_line_rx);
	s_prev_pad = PadRead(0);
	s_cool = 0;
	for (;;) {
		mixnet_psx_pump();
		handle_pad();
		(void)mixnet_nav_render_screen(s_view, (size_t)sizeof s_view);
		mixnet_psx_blt_screen(s_view, (int)strlen(s_view));
		if (mixnet_nav_want_quit()) break;
	}
	PadStop();
	ResetGraph(3);
	StopCallback();
	return 0;
}
