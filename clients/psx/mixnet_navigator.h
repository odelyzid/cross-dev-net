/*
 * mixnet_psx: Netscape-style hub UI over the mixnet v0 line protocol.
 * All drawing is done via a text buffer; wire mixnet_tx_fn to your bridge, then
 * feed server lines to mixnet_nav_on_incoming_line and user keys to mixnet_nav_user_key.
 */
#ifndef MIXNET_PSX_NAVIGATOR_H
#define MIXNET_PSX_NAVIGATOR_H

#include "../include/mixnet_config.h"
#include "../common/mixnet_line.h"
#include <stddef.h> /* size_t */

#define MIXNET_NAV_TITLE  "Mixnet Navigator"
#define MIXNET_NAV_VER    "0.3-psx"
#define MIXNET_PSX_SCROLL 28

void mixnet_nav_init(mixnet_tx_fn link_tx, void* link_user);

/* Server -> UI (one complete line, no \r\n). */
void mixnet_nav_on_incoming_line(const char* line);

/*
 * User input: single control character or a text line.
 * - Menu hub: 1..6, h, g, p, s, c, f, a
 * - Raw protocol: if line looks like "HELLO x" or "MSG ..." pass through
 * - :commands: ":rooms" ":who" ":part" ":j room" ":m text" ":loc mixnet://..."
 * Returns: number of lines to send (0 or 1). out lines are \n terminated by mixnet_write_line caller?
 * Actually: fills out_send with a single protocol line to transmit (no \n required; sender may use mixnet_write_line).
 * Return 1 = one line in out_send, 0 = nothing, -1 = quit requested
 */
int mixnet_nav_user_key(
	const char* user_line,
	char* out_send,
	size_t out_cap
);

/* Serialize full "browser chrome" for debug / FntPrint (null-terminated). */
int mixnet_nav_render_screen(char* out, size_t cap);

void mixnet_nav_reset(void);

/* After :q / :quit, UI loop should break. */
int mixnet_nav_want_quit(void);

#endif
