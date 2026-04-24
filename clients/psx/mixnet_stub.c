/*
 * 68mixCross — PlayStation: line protocol + bridge hooks.
 * No retail TCP; route bytes through serial/USB bridge or homebrew net layer.
 * Compile with: common/mixnet_line.c and ../include (see README).
 */
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"
#include "../common/mixnet_line.h"
#include "../common/mixnet_line.c"

#include <stddef.h>
#include <string.h>

/* --- TX buffer (or replace with SIO/DMA writer) ----------------------- */

#define MIXNET_PSX_TX_CAP 600
static unsigned char psx_tx_buf[MIXNET_PSX_TX_CAP];
static unsigned psx_tx_len;

static void mixnet_psx_stub_tx(void* user, int byte) {
	(void)user;
	if (psx_tx_len < (unsigned)MIXNET_PSX_TX_CAP)
		psx_tx_buf[psx_tx_len++] = (unsigned char)byte;
}

static void mixnet_on_server_line(const char* line) {
	(void)line;
	/* Parse OK / ERR / INFO; draw with libgpu, debug font, etc. */
}

/* ---------------------------------------------------------------------- */

static int mixnet_psx_line_selftest(void) {
	mixnet_line_rx_t rx;
	char out[MIXNET_MAX_LINE + 4];
	int i;
	const char* sample = "INFO psx-mixnet\r\n";

	psx_tx_len = 0u;
	if (mixnet_write_line("JOIN lobby", mixnet_psx_stub_tx, NULL) != 0) return 0;
	if (psx_tx_len < 5u) return 0;
	if (psx_tx_buf[psx_tx_len - 1] != (unsigned char)'\n') return 0;

	mixnet_line_rx_init(&rx);
	for (i = 0; sample[i]; i++) {
		if (mixnet_line_rx_byte(&rx, (int)(unsigned char)sample[i], out, sizeof out)) {
			if (strcmp(out, "INFO psx-mixnet") == 0) {
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
	(void)MIXNET_DEFAULT_PORT;
	(void)MX_HELLO[0];
	/* InitGraph/ResetCallback/DSIO etc. go here for real hardware. */
	if (!mixnet_psx_line_selftest()) return 1;
	return 0;
}
