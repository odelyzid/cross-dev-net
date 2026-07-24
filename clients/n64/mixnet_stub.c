/*
 * 68mixCross — N64: packet protocol + bridge hooks (no TCP on cart).
 * Compile with: ../common/mixnet_packet.c and ../include (see README).
 *
 * Replace stub_tx with your UART/64Drive/PI FIFO writer; feed bytes to
 * mixnet_pkt_rx_byte(), then convert full packets to text via
 * mixnet_pkt_to_text() and dispatch in mixnet_on_server_line().
 */
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"
#include "../common/mixnet_packet.h"
#include "../common/mixnet_packet.c"

#include <stddef.h>
#include <string.h>

/* --- TX sink (swap for real hardware) --------------------------------- */

#define MIXNET_N64_TX_CAP 600
static unsigned char n64_tx_buf[MIXNET_N64_TX_CAP];
static unsigned n64_tx_len;

static void mixnet_n64_stub_tx(void* user, int byte) {
	(void)user;
	if (n64_tx_len < (unsigned)MIXNET_N64_TX_CAP)
		n64_tx_buf[n64_tx_len++] = (unsigned char)byte;
}

/* --- Your UI / bridge: one complete server line (null-terminated) ----- */

static void mixnet_on_server_line(const char* line) {
	/* Route OK / ERR / INFO / PRIVMSG to UI; ring buffer, overlay text, etc. */
	(void)line;
	/* e.g. strncmp(line, MX_P_OK, strlen(MX_P_OK)) */
}

/* ---------------------------------------------------------------------- */

static int mixnet_n64_packet_selftest(void) {
	mixnet_pkt_t tx, rx;
	mixnet_pkt_rx_t state;
	int i, ok = 0;
	char txt[MIXNET_MAX_LINE + 4];

	/* 1) Build a HELLO packet, serialize to stub buffer */
	mixnet_pkt_start(&tx, PKT_HELLO, 0);
	if (mixnet_pkt_append(&tx, "n64", 3) != 0) return 0;
	n64_tx_len = 0;
	if (mixnet_pkt_send(&tx, mixnet_n64_stub_tx, NULL) != 0) return 0;
	if (n64_tx_len < PKT_HEADER_SIZE) return 0;

	/* 2) Parse back from buffer */
	mixnet_pkt_rx_init(&state);
	for (i = 0; i < (int)n64_tx_len; i++) {
		int r = mixnet_pkt_rx_byte(&state, (int)n64_tx_buf[i], &rx);
		if (r == 1) { ok = 1; break; }
		if (r < 0) return 0;
	}
	if (!ok) return 0;

	/* 3) Verify round-trip */
	if (rx.hdr[0] != PKT_MAGIC) return 0;
	if (rx.hdr[1] != PKT_HELLO) return 0;
	if (rx.payload_len != 3) return 0;
	if (memcmp(rx.payload, "n64", 3) != 0) return 0;

	/* 4) Convert to text and back */
	if (mixnet_pkt_to_text(&rx, txt, sizeof txt) != 0) return 0;
	if (strcmp(txt, "HELLO n64") != 0) return 0;

	return 1;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	(void)MIXNET_DEFAULT_PORT;
	(void)MX_PING[0];

	if (!mixnet_n64_packet_selftest()) {
		/* In real firmware, log to debug (IS-Viewer) or show error screen. */
		return 1;
	}
	/* Real loop:
	 *   mixnet_pkt_rx_t rx;
	 *   mixnet_pkt_rx_init(&rx);
	 *   for (;;) {
	 *       int byte = read_from_link();
	 *       mixnet_pkt_t pkt;
	 *       int r = mixnet_pkt_rx_byte(&rx, byte, &pkt);
	 *       if (r == 1) {
	 *           char line[MIXNET_MAX_LINE];
	 *           mixnet_pkt_to_text(&pkt, line, sizeof line);
	 *           mixnet_on_server_line(line);
	 *       }
	 *   } */
	return 0;
}
