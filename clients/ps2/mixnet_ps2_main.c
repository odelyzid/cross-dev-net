/*
 * 68mixCross — PlayStation 2: Mixnet Navigator (Netscape-style hub for mixnetd).
 * Two transports over the ps2sdk network stack:
 *   - LAN:  direct TCP socket to mixnetd            (mixnet_ps2.elf <host> [port]...)
 *   - RELAY:udp "tty" channel via tools/ps2tty.py   (mixnet_ps2.elf -udp <host> [port]...)
 * Build:  make  (needs PS2DEV/PS2SDK toolchain + gsKit, see BUILD-PS2.md)
 * Reuses: mixnet_navigator.c (UI state machine) + common framing layers.
 */
#include "mixnet_ps2.h"
#include "mixnet_ps2_net.h"
#include "mixnet_ps2_ui.h"
#include "mixnet_ps2_link.h"
#include "mixnet_ps2_transport.h"
#include "../psx/mixnet_navigator.h"
#include "../include/mixnet_config.h"
#include "../common/mixnet_packet.h"
#include "../common/mixnet_line.h"

#include <kernel.h>
#include <delaythread.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char s_view[4096];

static void send_nav_line(const char* user_line) {
	char out[MIXNET_MAX_LINE + 4];
	int r = mixnet_nav_user_key(user_line, out, sizeof out);
	if (r == 1) {
		size_t n = strlen(out);
		size_t i;
		for (i = 0; i < n; i++) mixnet_ps2_link_tx(NULL, out[i]);
	}
}

int main(int argc, char** argv) {
	const char* host = MIXNET_PS2_DEFAULT_HOST;
	unsigned short port = (unsigned short)MIXNET_DEFAULT_PORT;
	const char* nick = NULL;
	const char* room = NULL;
	int udp_mode = 0;
	const struct mixnet_ps2_transport* tr;

	/* argv[1..]
	 *   mixnet_ps2.elf                     -> TCP, defaults
	 *   mixnet_ps2.elf <host> [port] [nick] [room]
	 *   mixnet_ps2.elf -udp <host> [port]
	 */
	if (argc >= 2) {
		if (strcmp(argv[1], "-udp") == 0) {
			udp_mode = 1;
			if (argc >= 3 && argv[2][0] != '-') host = argv[2];
			if (argc >= 4) port = (unsigned short)atoi(argv[3]);
		} else {
			host = argv[1];
			if (argc >= 3) port = (unsigned short)atoi(argv[2]);
			if (argc >= 4) nick = argv[3];
			if (argc >= 5) room = argv[4];
		}
	}
	(void)port; /* reserved for future modes */

	if (!mixnet_packet_selftest()) {
		scr_printf("mixnet_ps2: packet layer selftest FAILED\n");
		SleepThread();
		return 1;
	}

	if (mixnet_ps2_net_init() != 0) {
		scr_printf("mixnet_ps2: %s\n", mixnet_ps2_net_status());
		SleepThread();
		return 1;
	}

	if (mixnet_ps2_ui_init() != 0) {
		scr_printf("mixnet_ps2: ui init failed\n");
		SleepThread();
		return 1;
	}

	if (udp_mode) {
		mixnet_ps2_udp_set(host, (unsigned short)MIXNET_PS2_UDP_PORT);
		tr = &mixnet_ps2_udp_transport;
	} else {
		mixnet_ps2_tcp_set(host, port);
		tr = &mixnet_ps2_tcp_transport;
	}
	mixnet_ps2_link_bind(tr);

	mixnet_nav_init(mixnet_ps2_link_tx, NULL);
	tr->start();

	/* auto-join when nick+room given on the command line (ps2link execee argv) */
	if (!udp_mode && nick && room) {
		char b[MIXNET_MAX_LINE];
		snprintf(b, sizeof b, "HELLO %s", nick);
		send_nav_line(b);
		snprintf(b, sizeof b, "JOIN %s", room);
		send_nav_line(b);
	} else if (udp_mode) {
		scr_printf("mixnet_ps2: relay mode, waiting for ps2tty.py on %s\n", host);
	}

	for (;;) {
		int len;

		tr->poll();
		mixnet_ps2_ui_pad_poll();

		len = mixnet_nav_render_screen(s_view, (size_t)sizeof s_view);
		mixnet_ps2_ui_frame(s_view, len, tr->up());

		if (mixnet_nav_want_quit()) {
			int k;
			send_nav_line("QUIT");
			for (k = 0; k < 20; k++) {  /* let the final datagram/bytes out */
				tr->poll();
				DelayThread(16 * 1000);
			}
			break;
		}
	}

	tr->stop();
	SleepThread();
	return 0;
}