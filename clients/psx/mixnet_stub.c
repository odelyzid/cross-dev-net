/*
 * 68mixCross — PlayStation: Mixnet Navigator (Netscape-style hub for mixnetd).
 * Build:  mixnet_navigator.c + mixnet_stub.c  (+ common/mixnet_line linked via navigator)
 *   ccpsx -c -I. mixnet_navigator.c
 *   ccpsx -c -I. mixnet_stub.c
 *   (link; wire mixnet_line TX to your byte bridge, RX drives mixnet_line_rx_byte + mixnet_nav_on_incoming_line)
 */
#include "mixnet_psx.h"
#include "mixnet_navigator.h"
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"
#include "../common/mixnet_line.h"
#include "../common/mixnet_line.c"

#include <string.h>

#define PSX_TX_CAP 2048
static unsigned char s_psx_tx[PSX_TX_CAP];
static unsigned s_psx_len;

/* Bridge this to parallel/SIO/PC: host reads this buffer and forward to mixnetd TCP, or in-process test. */
static void mixnet_psx_link_tx(void* u, int byte) {
	(void)u;
	if (s_psx_len < (unsigned)PSX_TX_CAP - 1u) s_psx_tx[s_psx_len++] = (unsigned char)byte;
}

/* Default no-ops: override in your project with FntLoad / FntPrint / VSync. */
void mixnet_psx_init_video(void) {}

void mixnet_psx_pump(void) {}

void mixnet_psx_blt_screen(const char* text, int n_bytes) {
	(void)text;
	(void)n_bytes;
}

/* ---- line layer smoke test (unchanged idea) ---- */
static int line_layer_selftest(void) {
	mixnet_line_rx_t rx;
	char out[MIXNET_MAX_LINE + 4];
	int i;
	const char* sample = "INFO line-test\r\n";
	s_psx_len = 0u;
	if (mixnet_write_line("JOIN x", mixnet_psx_link_tx, NULL) != 0) return 0;
	if (s_psx_len < 4u) return 0;
	mixnet_line_rx_init(&rx);
	for (i = 0; sample[i]; i++) {
		if (mixnet_line_rx_byte(&rx, (int)(unsigned char)sample[i], out, sizeof out)) {
			if (strcmp(out, "INFO line-test") == 0) return 1;
		}
	}
	return 0;
}

/* ---- in-memory hub demo (no hardware bridge) ---- */
static int navigator_demo(void) {
	char view[4096];
	mixnet_nav_init(mixnet_psx_link_tx, NULL);
	mixnet_nav_on_incoming_line("INFO mixnetd -- session abcd");
	mixnet_nav_on_incoming_line("OK hello 0000000000000001");
	mixnet_nav_on_incoming_line("OK join lobby");
	mixnet_nav_on_incoming_line("PRIVMSG other Welcome to the hub");
	(void)mixnet_nav_user_key(":h", view, sizeof view);
	if (mixnet_nav_render_screen(view, sizeof view) < 20) return 0;
	if (!strstr(view, "Mixnet Navigator")) return 0;
	if (!strstr(view, "PRIVMSG other")) return 0;
	return 1;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	(void)MIXNET_DEFAULT_PORT;
	(void)MX_HELLO[0];
	if (!line_layer_selftest()) return 1;
	if (!navigator_demo()) return 2;
	/*
	 * Production loop (pseudo):
	 *   mixnet_psx_init_video();
	 *   mixnet_nav_init(mixnet_psx_link_tx, NULL);
	 *   for (;;) {
	 *     while (read_byte_from_link(&b))
	 *       if (mixnet_line_rx_byte(&rx, b, linebuf, sizeof linebuf))
	 *         mixnet_nav_on_incoming_line(linebuf);
	 *     if (pad & START) { feed user string from on-screen keyboard; mixnet_nav_user_key(...); }
	 *     mixnet_nav_render_screen(screen, sizeof screen);
	 *     mixnet_psx_blt_screen(screen, len);
	 *     mixnet_psx_pump();
	 *     if (mixnet_nav_want_quit()) break;
	 *   }
	 */
	return 0;
}
