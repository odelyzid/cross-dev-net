/*
 * mixnet_ps2_ui: gsKit renderer + pad for the Mixnet PS2 client.
 * Font: embedded 8x8 bitmap (mixnet_ps2_font.h) compiled into one CT32
 * texture, drawn with gsKit_prim_sprite_texture (16x16 px per char).
 */
#include "mixnet_ps2_ui.h"
#include "mixnet_ps2_font.h"
#include "../psx/mixnet_navigator.h"
#include <gsKit.h>
#include <gsToolkit.h>
#include <libpad.h>
#include <kernel.h>
#include <delaythread.h>
#include <stdlib.h>
#include <string.h>

#define PS2_UI_FONT_TEX_W 320
#define PS2_UI_FONT_TEX_H 32

typedef struct {
	GSGLOBAL* gs;
	GSTEXTURE ftex;
	unsigned char padbuf[256] __attribute__((aligned(64)));
	unsigned short prev;
	int cool;
	int inited;
} mixnet_ps2_ui_t;

static mixnet_ps2_ui_t s_ui;

#define UI_COL_TEXT  GS_SETREG_RGBAQ(0xE0, 0xE8, 0xF0, 0x80, 0)
#define UI_COL_DIM   GS_SETREG_RGBAQ(0x50, 0x58, 0x60, 0x80, 0)
#define UI_COL_RED   GS_SETREG_RGBAQ(0xC0, 0x20, 0x20, 0x80, 0)

static void ui_build_font(void) {
	unsigned char* px;
	int x, y, gi, b;
	size_t a;

	px = (unsigned char*)malloc((size_t)PS2_UI_FONT_TEX_W * PS2_UI_FONT_TEX_H * 4);
	if (!px) return;
	memset(px, 0, (size_t)PS2_UI_FONT_TEX_W * PS2_UI_FONT_TEX_H * 4);

	for (gi = 0; gi < MIXNET_PS2_FONT_COUNT; gi++) {
		int gx = (gi & 31);
		int gy = (gi >> 5);
		for (y = 0; y < MIXNET_PS2_FONT_H; y++) {
			for (x = 0; x < MIXNET_PS2_FONT_W; x++) {
				b = (mixnet_ps2_font8x8[gi][y] >> (7 - x)) & 1;
				if (!b) continue;
				a = (size_t)((gy * MIXNET_PS2_FONT_H + y) * PS2_UI_FONT_TEX_W +
				             (gx * MIXNET_PS2_FONT_W + x)) * 4;
				px[a + 0] = 0xFF;
				px[a + 1] = 0xFF;
				px[a + 2] = 0xFF;
				px[a + 3] = 0xFF;
			}
		}
	}

	s_ui.ftex.Width = PS2_UI_FONT_TEX_W;
	s_ui.ftex.Height = PS2_UI_FONT_TEX_H;
	s_ui.ftex.PSM = GS_PSM_CT32;
	s_ui.ftex.Filter = GS_FILTER_NEAREST;
	s_ui.ftex.Mem = (u32*)px;
	s_ui.ftex.Vram = gsKit_vram_alloc(s_ui.gs, (u32)(PS2_UI_FONT_TEX_W * PS2_UI_FONT_TEX_H * 4),
	                                  GSKIT_ALLOC_USERBUFFER);
	gsKit_texture_upload(s_ui.gs, &s_ui.ftex);
	free(px);
}

int mixnet_ps2_ui_init(void) {
	struct padButtonStatus st;
	int tries;

	if (s_ui.inited) return 0;

	memset(&s_ui, 0, sizeof s_ui);

	s_ui.gs = gsKit_init_global();
	if (!s_ui.gs) return -1;
	s_ui.gs->Mode = gsKit_detect_signal();
	s_ui.gs->Interlace = GS_NONINTERLACED;
	s_ui.gs->Field = GS_FIELD_NORMAL;
	s_ui.gs->Width = 640;
	s_ui.gs->Height = 448;
	s_ui.gs->Aspect = GS_ASPECT_4_3;
	s_ui.gs->PSM = GS_PSM_CT32;
	s_ui.gs->PSMZ = GS_PSMZ_16S;
	gsKit_init_screen(s_ui.gs);
	gsKit_vram_clear(s_ui.gs);
	gsKit_set_test(s_ui.gs, GS_ZTEST_OFF);
	gsKit_set_primalpha(s_ui.gs, GS_SETREG_ALPHA(0, 0, 0, 0, 0), 0);
	gsKit_set_clamp(s_ui.gs, GS_CMODE_CLAMP);

	ui_build_font();

	/* ---- pad ---- */
	padInit(0);
	padPortOpen(0, 0, s_ui.padbuf);
	for (tries = 0; tries < 60; tries++) {
		if (padGetState(0, 0) == PAD_STATE_STABLE) break;
		DelayThread(100 * 1000);
	}
	s_ui.prev = 0;
	if (padRead(0, 0, &st)) s_ui.prev = st.btns;

	s_ui.inited = 1;
	return 0;
}

/* One row of text at grid position. */
static void ui_text_row(GSGLOBAL* gs, GSTEXTURE* ft, int row, const char* s, int n, u64 color) {
	float x, y;
	int i;
	char c;
	int z = 0;

	if (row >= MIXNET_PS2_GRID_H) return;
	y = (float)(8 + row * MIXNET_PS2_FONT_H * MIXNET_PS2_FONT_Y);
	for (i = 0, x = 8.0f; i < n && x < 640.0f - 8.0f; i++) {
		c = s[i];
		if (c < MIXNET_PS2_FONT_FIRST || c > 126) c = ' ';
		{
			int g = (int)c - MIXNET_PS2_FONT_FIRST;
			float u0 = (float)((g & 31) * MIXNET_PS2_FONT_W);
			float v0 = (float)((g >> 5) * MIXNET_PS2_FONT_H);
			gsKit_prim_sprite_texture(gs, ft, x, y, u0, v0,
			                          x + (float)(MIXNET_PS2_FONT_W * MIXNET_PS2_FONT_X),
			                          y + (float)(MIXNET_PS2_FONT_H * MIXNET_PS2_FONT_Y),
			                          u0 + (float)MIXNET_PS2_FONT_W,
			                          v0 + (float)MIXNET_PS2_FONT_H,
			                          z, color);
		}
		x += (float)(MIXNET_PS2_FONT_W * MIXNET_PS2_FONT_X);
	}
}

void mixnet_ps2_ui_frame(const char* view, int len, int connected) {
	const char* end;
	const char* p;
	int row;

	if (!s_ui.inited) return;
	if (!view || len < 0) len = 0;

	gsKit_clear(s_ui.gs, GS_SETREG_RGBAQ(0x08, 0x0C, 0x10, 0x80, 0));

	row = 0;
	end = view + len;
	p = view;
	while (p < end && row < MIXNET_PS2_GRID_H) {
		const char* nl = (const char*)memchr(p, '\n', (size_t)(end - p));
		int n = (int)((nl ? nl : end) - p);
		char line[77];
		int i;
		if (n > MIXNET_PS2_GRID_W) n = MIXNET_PS2_GRID_W;
		for (i = 0; i < n; i++) {
			char c = p[i];
			line[i] = (c >= 32 && c != 127) ? c : ' ';
		}
		line[n] = '\0';
		ui_text_row(s_ui.gs, &s_ui.ftex, row, line, n, UI_COL_TEXT);
		row++;
		p = nl ? nl + 1 : end;
		while (p < end && (*p == '\r')) p++;
	}

	/* status row */
	if (row < MIXNET_PS2_GRID_H) {
		ui_text_row(s_ui.gs, &s_ui.ftex, row, connected ? "MIXNET PS2 - online" : "MIXNET PS2 - no link",
		            24, connected ? UI_COL_DIM : UI_COL_RED);
	}

	/* red border while disconnected */
	if (!connected) {
		gsKit_prim_sprite(s_ui.gs, 4.0f, 4.0f, 636.0f, 6.0f, 0, UI_COL_RED);
		gsKit_prim_sprite(s_ui.gs, 4.0f, 442.0f, 636.0f, 444.0f, 0, UI_COL_RED);
		gsKit_prim_sprite(s_ui.gs, 4.0f, 4.0f, 6.0f, 444.0f, 0, UI_COL_RED);
		gsKit_prim_sprite(s_ui.gs, 634.0f, 4.0f, 636.0f, 444.0f, 0, UI_COL_RED);
	}

	gsKit_queue_exec(s_ui.gs);
	gsKit_sync_flip(s_ui.gs);
	gsKit_queue_reset(s_ui.gs->Per_Queue);
}

void mixnet_ps2_ui_pad_poll(void) {
	struct padButtonStatus st;
	unsigned short p, edge;

	if (!s_ui.inited) return;
	if (s_ui.cool > 0) {
		s_ui.cool--;
		return;
	}
	if (!padRead(0, 0, &st)) return;
	p = st.btns;
	edge = (unsigned short)(p & ~s_ui.prev);
	s_ui.prev = p;

	if (edge & PAD_UP)    (void)mixnet_nav_user_key("1", NULL, 0);
	if (edge & PAD_DOWN)  (void)mixnet_nav_user_key("2", NULL, 0);
	if (edge & PAD_RIGHT) (void)mixnet_nav_user_key("3", NULL, 0);
	if (edge & PAD_LEFT)  (void)mixnet_nav_user_key("4", NULL, 0);
	if (edge & PAD_L1)    (void)mixnet_nav_user_key("5", NULL, 0);
	if (edge & PAD_L2)    (void)mixnet_nav_user_key("6", NULL, 0);
	if (edge & PAD_SELECT)(void)mixnet_nav_user_key(":h", NULL, 0);
	if (edge & PAD_START) (void)mixnet_nav_user_key(":g", NULL, 0);
	if (edge & PAD_R1)    (void)mixnet_nav_user_key("PING", NULL, 0);
	if (edge) s_ui.cool = 6;
}