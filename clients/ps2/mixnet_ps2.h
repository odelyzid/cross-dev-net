/* mixnet_ps2: PS2 client version/config (see README in this directory). */
#ifndef MIXNET_PS2_H
#define MIXNET_PS2_H

#define MIXNET_PS2_VER     "0.1-ps2"
#define MIXNET_PS2_TITLE   "Mixnet PS2 Navigator"

/* Default mixnetd server IP when no argv is passed (ps2link execee). */
#define MIXNET_PS2_DEFAULT_HOST "192.168.0.100"

/* Shared UDP relay port (both ends: PS2 app and tools/ps2tty.py). */
#define MIXNET_PS2_UDP_PORT 19678

#endif