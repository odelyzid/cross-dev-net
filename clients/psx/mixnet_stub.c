/*
 * 68mixCross client stub — Sony PlayStation (MIPS, R3000)
 *
 * Options for real I/O (none implemented here):
 *   - **serial** to PC bridge (Yaroze-era PC link, or modern parallel/USB adapters),
 *   - **Xplorer / Caetla**-style backdoor to push bytes to a host,
 *   - **PSIO / X-Station** + custom payload (community-dependent),
 *   - **Net Yaroze**-style if you still have a serial pipe to a UNIX box (rare).
 *
 * The wire format is still line-delimited ASCII; mixnetd runs on a modern host, bridge forwards lines.
 */
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"
int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	(void)MIXNET_DEFAULT_PORT;
	(void)MX_HELLO[0];
	/* TODO: InitGraph / ResetCallback as per your psyq/ps2sdk-ps1-style project; add socket or serial bridge. */
	return 0;
}
