/* mixnet_ps2_link: transport <-> navigator glue.
 * Feeds the raw byte stream through the same auto-detect framing the PS1
 * serial stub uses (0x58 -> binary packet, else v0 text line), and forwards
 * navigator TX bytes to the active transport. Byte-trampoline kept here so
 * transports stay UI-free.
 */
#ifndef MIXNET_PS2_LINK_H
#define MIXNET_PS2_LINK_H

#include "mixnet_ps2_transport.h"

/* Bind the active transport (call after setting IP/port and before loop). */
void mixnet_ps2_link_bind(const struct mixnet_ps2_transport* tr);

/* Feed one received byte from the transport. */
void mixnet_ps2_link_feed(int byte);

/* TX trampoline for mixnet_tx_fn signatures. */
void mixnet_ps2_link_tx(void* user, int byte);

#endif