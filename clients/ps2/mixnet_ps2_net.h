/* mixnet_ps2_net: modern ps2sdk network init for the Mixnet PS2 client.
 * Boots the IOP, loads DEV9/NETMAN/SMAP/PS2IP-NM from embedded IRX arrays
 * (bin2c generated at build time), configures a static IP and waits for
 * the Ethernet link. Mirrors samples/network/tcpip-basic.
 */
#ifndef MIXNET_PS2_NET_H
#define MIXNET_PS2_NET_H

/* PS2 static network config (edit to taste, or override with argv defaults). */
#define MIXNET_PS2_IP   "192.168.0.80"
#define MIXNET_PS2_NM   "255.255.255.0"
#define MIXNET_PS2_GW   "192.168.0.1"

/* Link-layer status string (scr_printf / status bar use). */
const char* mixnet_ps2_net_status(void);

/* Returns 0 on success. On failure mixnet_ps2_net_status() explains. */
int mixnet_ps2_net_init(void);

#endif