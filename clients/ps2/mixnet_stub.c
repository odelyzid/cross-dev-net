/*
 * 68mixCross client stub — Sony PlayStation 2 (R5900 / EE)
 *
 * EE-side **TCP** is possible in homebrew (lwIP, ps2ip, or SDK stacks), but this repo
 * does not pull those in. For bring-up, same pattern as other consoles: **line framing**
 * to a **PC bridge** or add **sce* socket**-style code when your SDK is fixed.
 */
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	(void)MIXNET_DEFAULT_PORT;
	(void)MX_P_ERR[0];
	/* TODO: sce* init (depends on: ps2dev / PS2SDK / wLaunchELF embed, etc.) */
	return 0;
}
