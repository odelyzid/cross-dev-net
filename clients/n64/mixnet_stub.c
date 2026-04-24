/*
 * 68mixCross — N64: line protocol + bridge hooks (no TCP on cart).
 * Compile with: common/mixnet_line.c and ../include (see README).
 *
 * Replace stub_tx with your UART/64Drive/PI FIFO writer; call mixnet_line_rx_byte
 * for each received byte, then handle full lines in mixnet_on_server_line.
 */
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"
#include "../common/mixnet_line.h"
#include "../common/mixnet_line.c"

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

/* --- Your UI / bridge: one complete server line (no \r\n) ------------ */

static void mixnet_on_server_line(const char* line) {
	/* Route OK / ERR / INFO / PRIVMSG to UI; ring buffer, overlay text, etc. */
	(void)line;
	/* e.g. strncmp(line, MX_P_OK, strlen(MX_P_OK)) */
}

/* ---------------------------------------------------------------------- */

static int mixnet_n64_line_selftest(void) {
	mixnet_line_rx_t rx;
	char out[MIXNET_MAX_LINE + 4];
	int i;
	const char* sample = "OK n64-mixnet\r\n";
	/* 1) TX: HELLO should end with \n */
	n64_tx_len = 0u;
	if (mixnet_write_line("HELLO n64", mixnet_n64_stub_tx, NULL) != 0) return 0;
	if (n64_tx_len < 1u) return 0;
	if (n64_tx_buf[n64_tx_len - 1] != (unsigned char)'\n') return 0;
	/* 2) RX: feed "OK ...\n" */
	mixnet_line_rx_init(&rx);
	for (i = 0; sample[i]; i++) {
		if (mixnet_line_rx_byte(&rx, (int)(unsigned char)sample[i], out, sizeof out)) {
			if (strcmp(out, "OK n64-mixnet") == 0) {
				mixnet_on_server_line(out);
				return 1;
			}
		}
	}
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	(void)MIXNET_DEFAULT_PORT; /* used by host bridge config, not on-cart */
	(void)MX_PING[0];

	if (!mixnet_n64_line_selftest()) {
		/* In real firmware, log to debug (IS-Viewer) or show error screen. */
		return 1;
	}
	/* Real loop: for (;;) { read byte from link; if (rx_byte(...)) on_server_line(out); } */
	return 0;
}
