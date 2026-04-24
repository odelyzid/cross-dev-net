/* Shared limits for 68mixCross v0 line protocol (see .cursor/.documentation/cross-net/protocol-v0.mdc) */
#ifndef MIXNET_CONFIG_H
#define MIXNET_CONFIG_H

#define MIXNET_DEFAULT_PORT 19677u
#define MIXNET_MAX_LINE 512u

/* PS1 has no IP stack: host/port label the *logical* target for the PC bridge (TCP to mixnetd). */
#ifndef MIXNET_DEFAULT_HOST
#define MIXNET_DEFAULT_HOST "127.0.0.1"
#endif
/* Empty string = no default room; set e.g. "lobby" to match quick :g in docs */
#ifndef MIXNET_DEFAULT_ROOM
#define MIXNET_DEFAULT_ROOM "lobby"
#endif

#endif
