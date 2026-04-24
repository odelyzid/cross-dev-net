/*
 * 68mixCross client stub — Nintendo 64
 *
 * The retail N64 has no built-in socket stack. You must choose one of:
 *   - PC / Raspberry Pi **bridge** (USB or parallel port) carrying line-framed data (see clients/genesis/mixnet_line.c),
 *   - **homebrew** + flashcart (e.g. 64Drive, EverDrive) with OS serial or custom USB (libdragon),
 *   - Or tunnel over **controller** / **Rumble Pak** (not serious for chat — academic only).
 *
 * Link: libultra and your SDK entry; replace main() with a proper init + loop when you add I/O.
 */
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	/* TODO: init video/debug (IS Viewer, 64Drive USB, etc.); then drive mixnet line protocol over the chosen link. */
	(void)MIXNET_DEFAULT_PORT;
	(void)MX_PING[0];
	return 0;
}
