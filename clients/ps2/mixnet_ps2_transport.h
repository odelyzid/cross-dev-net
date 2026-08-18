/* mixnet_ps2_transport.h: pluggable byte-stream transports for the PS2 client.
 * Both the LAN (TCP) and relay ("serial-style" UDP/tty) transports present the
 * same interface: connect, byte TX, poll pump, status. The main loop feeds a
 * shared link_tx into navigation, exactly like the PS1 serial stub.
 */
#ifndef MIXNET_PS2_TRANSPORT_H
#define MIXNET_PS2_TRANSPORT_H

struct mixnet_ps2_transport {
	void (*start)(void);
	void (*poll)(void);        /* pump RX; must be called every frame */
	void (*tx)(int byte);
	int  (*up)(void);          /* 1 = link online now */
	const char* (*status)(void);
	void (*stop)(void);
};

extern const struct mixnet_ps2_transport mixnet_ps2_tcp_transport;
extern const struct mixnet_ps2_transport mixnet_ps2_udp_transport;

/* Configure before start(). s_tcp: char IP + port. s_udp: relay IP + port. */
void mixnet_ps2_tcp_set(const char* ip, unsigned short port);
void mixnet_ps2_udp_set(const char* ip, unsigned short port);

#endif