/*
 * mixnet_ps2_udp: relay transport — the "serial-style" channel.
 * The PS2 talks packet-style UDP datagrams (max ~1400 bytes each) with the
 * host-side relay (tools/ps2tty.py), which pipes them to mixnetd over TCP.
 * Datagram boundaries are irrelevant to the mixnet framing layers, so the
 * relay just concatenates; this mirrors how a serial/UART byte pipe behaves.
 * Port MIXNET_PS2_UDP_PORT (19678) is used on BOTH sides.
 */
#include "mixnet_ps2_transport.h"
#include "mixnet_ps2_link.h"
#include "../include/mixnet_config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <kernel.h>
#include <debug.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define UDP_TX_CAP 1024

static char s_ip[64];
static unsigned short s_port;
static int s_sock = -1;
static int s_peer_ok;
static char s_status[96];
static unsigned char s_txbuf[UDP_TX_CAP];
static int s_txlen;

static const char* udp_status(void) { return s_status; }

static void udp_note(const char* s) {
	strncpy(s_status, s, sizeof s_status - 1);
	s_status[sizeof s_status - 1] = '\0';
}

static void udp_close(void) {
	if (s_sock >= 0) {
		close(s_sock);
		s_sock = -1;
	}
	s_peer_ok = 0;
}

static void udp_start(void) {
	struct sockaddr_in sa;
	int one = 1;

	if (s_sock >= 0) return;

	s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s_sock < 0) { udp_note("udp socket() failed"); return; }
	setsockopt(s_sock, SOL_SOCKET, SO_BROADCAST, (void*)&one, sizeof one);

	/* Bind the well-known relay port on this end. */
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(s_port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s_sock, (struct sockaddr*)&sa, sizeof sa) < 0) {
		udp_note("udp bind failed");
		udp_close();
		return;
	}
	udp_note("udp relay: waiting for traffic");
}

static void udp_poll(void) {
	struct sockaddr_in sa;
	unsigned char buf[UDP_TX_CAP];
	socklen_t slen;
	int n;
	fd_set rfds;
	struct timeval tv;

	if (s_sock < 0) return;

	FD_ZERO(&rfds);
	FD_SET(s_sock, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	if (select(s_sock + 1, &rfds, NULL, NULL, &tv) <= 0) goto flush;

	slen = sizeof sa;
	n = recvfrom(s_sock, buf, sizeof buf, 0, (struct sockaddr*)&sa, &slen);
	if (n > 0) {
		int i;
		if (!s_peer_ok) {
			scr_printf("mixnet_ps2: udp relay peer %u.%u.%u.%u:%u\n",
			           (unsigned)(ntohl(sa.sin_addr.s_addr) >> 24) & 0xff,
			           (unsigned)(ntohl(sa.sin_addr.s_addr) >> 16) & 0xff,
			           (unsigned)(ntohl(sa.sin_addr.s_addr) >> 8) & 0xff,
			           (unsigned)(ntohl(sa.sin_addr.s_addr)) & 0xff,
			           (unsigned)ntohs(sa.sin_port));
			s_peer_ok = 1;
			udp_note("online (udp relay)");
		}
		for (i = 0; i < n; i++) mixnet_ps2_link_feed(buf[i]);
	}

flush:
	/* flush buffered TX as one datagram per frame */
	if (s_txlen > 0) {
		struct sockaddr_in peer;
		memset(&peer, 0, sizeof peer);
		peer.sin_family = AF_INET;
		peer.sin_port = htons(s_port);
		peer.sin_addr.s_addr = inet_addr(s_ip);
		if (sendto(s_sock, s_txbuf, s_txlen, 0, (struct sockaddr*)&peer, sizeof peer) < 0)
			udp_note("udp sendto failed");
		s_txlen = 0;
	}
}

static void udp_tx(int byte) {
	if (s_txlen < (int)sizeof s_txbuf)
		s_txbuf[s_txlen++] = (unsigned char)(byte & 0xff);
	/* overflow: drop oldest (should never happen; frames are << 1K) */
	else
		memmove(s_txbuf, s_txbuf + 1, (size_t)(s_txlen - 1));
}

static int udp_up(void) { return s_sock >= 0 && s_peer_ok; }

static void udp_stop(void) { udp_close(); }

void mixnet_ps2_udp_set(const char* ip, unsigned short port) {
	strncpy(s_ip, ip, sizeof s_ip - 1);
	s_ip[sizeof s_ip - 1] = '\0';
	s_port = port;
	udp_note("udp relay: idle");
}

const struct mixnet_ps2_transport mixnet_ps2_udp_transport = {
	udp_start, udp_poll, udp_tx, udp_up, udp_status, udp_stop
};