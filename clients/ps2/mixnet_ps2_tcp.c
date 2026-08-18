/*
 * mixnet_ps2_tcp: LAN transport — direct TCP socket to mixnetd.
 * Non-blocking connect via select(); per-frame select() poll + recv.
 */
#include "mixnet_ps2_transport.h"
#include "mixnet_ps2_link.h"
#include "../include/mixnet_config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <kernel.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TCP_RECV_BUF 1024

static char s_ip[64];
static unsigned short s_port;
static int s_sock = -1;
static int s_trying;
static int s_retry_timer;
static char s_status[96];

static const char* tcp_status(void) { return s_status; }

static void tcp_note(const char* s) {
	strncpy(s_status, s, sizeof s_status - 1);
	s_status[sizeof s_status - 1] = '\0';
}

static void tcp_close(void) {
	if (s_sock >= 0) {
		shutdown(s_sock, SHUT_RDWR);
		close(s_sock);
		s_sock = -1;
	}
}

/* Caller must provide a valid dotted-quad IP (no DNS on ps2ip). */
static int ip_parse(const char* text, unsigned int* out) {
	unsigned int a, b, c, d;
	if (sscanf(text, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return -1;
	if (a > 255 || b > 255 || c > 255 || d > 255) return -1;
	*out = (a << 24) | (b << 16) | (c << 8) | d;
	return 0;
}

static void tcp_try_connect(void) {
	struct sockaddr_in sa;
	unsigned int ip;
	fd_set wfds;
	struct timeval tv;

	if (s_sock >= 0) return;

	if (ip_parse(s_ip, &ip) != 0) {
		tcp_note("bad server IP");
		return;
	}

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(s_port);
	sa.sin_addr.s_addr = htonl(ip);

	s_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (s_sock < 0) { tcp_note("socket() failed"); return; }

	if (connect(s_sock, (struct sockaddr*)&sa, sizeof sa) < 0) {
		tcp_close();
		tcp_note("connect failed (retrying)");
		s_retry_timer = 120;
		return;
	}

	/* lwIP connect is async: wait for completion, capped. */
	FD_ZERO(&wfds);
	FD_SET(s_sock, &wfds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (select(s_sock + 1, NULL, &wfds, NULL, &tv) <= 0) {
		tcp_close();
		tcp_note("connect timeout (retrying)");
		s_retry_timer = 120;
		return;
	}

	s_trying = 0;
	tcp_note("online (TCP)");
	scr_printf("mixnet_ps2: TCP connected to %s:%u\n", s_ip, s_port);
}

static void tcp_start(void) {
	if (s_sock < 0) {
		s_trying = 1;
		tcp_try_connect();
	} else if (!s_trying) {
		/* already online: nothing to do */
	}
}

static void tcp_poll(void) {
	fd_set rfds;
	struct timeval tv;
	unsigned char buf[TCP_RECV_BUF];
	int n;

	if (s_sock < 0) {
		if (s_retry_timer > 0 && --s_retry_timer == 0) tcp_try_connect();
		return;
	}

	FD_ZERO(&rfds);
	FD_SET(s_sock, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if (select(s_sock + 1, &rfds, NULL, NULL, &tv) <= 0) return;

	n = recv(s_sock, buf, sizeof buf, 0);
	if (n <= 0) {
		scr_printf("mixnet_ps2: TCP link lost\n");
		tcp_close();
		tcp_note("link lost (retrying)");
		s_retry_timer = 120;
		return;
	}
	{
		int i;
		for (i = 0; i < n; i++) mixnet_ps2_link_feed(buf[i]);
	}
}

static void tcp_tx(int byte) {
	char c = (char)(byte & 0xff);
	if (s_sock < 0) return;
	if (send(s_sock, &c, 1, 0) < 0) {
		tcp_close();
		tcp_note("send failed (retrying)");
		s_retry_timer = 60;
	}
}

static int tcp_up(void) { return s_sock >= 0 && !s_trying; }

static void tcp_stop(void) { tcp_close(); }

void mixnet_ps2_tcp_set(const char* ip, unsigned short port) {
	strncpy(s_ip, ip, sizeof s_ip - 1);
	s_ip[sizeof s_ip - 1] = '\0';
	s_port = port;
	tcp_note("connecting...");
}

const struct mixnet_ps2_transport mixnet_ps2_tcp_transport = {
	tcp_start, tcp_poll, tcp_tx, tcp_up, tcp_status, tcp_stop
};