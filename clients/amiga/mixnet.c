/*
 * Amiga 500 / 2000+ — TCP line client (AmigaOS 3.x + BSD stack + pthreads).
 * Build (example, toolchain-specific):
 *   m68k-amigaos-gcc -O2 -std=c99 -o mixnet mixnet.c ../common/mixnet_packet.c -lpthread -lsocket
 * Roadshow/AmiTCP: add vendor include/lib.
 */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"
#include "../common/mixnet_packet.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int g_sock = -1;
static bool g_stop;

static void* reader_main(void* arg) {
	(void)arg;
	mixnet_pkt_rx_t pkt_rx;
	int pkt_mode = 0; /* 0 = detect, 1 = binary packet, 2 = v0 text */
	char line_buf[MIXNET_MAX_LINE + 4];
	int line_pos = 0;
	mixnet_pkt_rx_init(&pkt_rx);

	for (;;) {
		if (g_stop) break;
		unsigned char b;
		ssize_t n = recv(g_sock, &b, 1, 0);
		if (n <= 0) break;

		/* auto-detect on first byte from server */
		if (pkt_mode == 0) {
			pkt_mode = (b == PKT_MAGIC) ? 1 : 2;
			if (pkt_mode == 1)
				mixnet_pkt_rx_init(&pkt_rx);
		}

		if (pkt_mode == 1) {
			mixnet_pkt_t pkt;
			int r = mixnet_pkt_rx_byte(&pkt_rx, (int)b, &pkt);
			if (r == 1) {
				char txt[MIXNET_MAX_LINE + 4];
				if (mixnet_pkt_to_text(&pkt, txt, sizeof txt) == 0) {
					puts(txt);
					fflush(stdout);
				}
			} else if (r < 0) {
				mixnet_pkt_rx_init(&pkt_rx);
			}
		} else {
			if (b == '\r') continue;
			if (b == '\n') {
				line_buf[line_pos] = '\0';
				puts(line_buf);
				fflush(stdout);
				line_pos = 0;
				continue;
			}
			if (line_pos < (int)sizeof(line_buf) - 2)
				line_buf[line_pos++] = (char)b;
			else
				line_pos = 0;
		}
	}
	return NULL;
}

static int connect_host(const char* host, unsigned short port) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	char portstr[16];
	snprintf(portstr, sizeof portstr, "%u", (unsigned)port);
	struct addrinfo* res = NULL;
	if (getaddrinfo(host, portstr, &hints, &res) != 0 || !res) {
		fputs("getaddrinfo failed\n", stderr);
		return -1;
	}
	int fd = -1;
	for (struct addrinfo* p = res; p; p = p->ai_next) {
		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fd < 0) continue;
		if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
		close(fd);
		fd = -1;
	}
	freeaddrinfo(res);
	if (fd < 0) {
		fputs("connect failed\n", stderr);
		return -1;
	}
	g_sock = fd;
	return 0;
}

static int send_line(const char* s) {
	char buf[MIXNET_MAX_LINE + 2];
	size_t n = strlen(s);
	if (n > MIXNET_MAX_LINE) return -1;
	memcpy(buf, s, n);
	if (n == 0 || s[n - 1] != '\n')
		buf[n++] = '\n';
	size_t sent = 0;
	while (sent < n) {
		ssize_t w = send(g_sock, buf + sent, n - sent, 0);
		if (w <= 0) return -1;
		sent += (size_t)w;
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* host = "127.0.0.1";
	unsigned short port = (unsigned short)MIXNET_DEFAULT_PORT;
	const char* nick = NULL;
	const char* room = NULL;
	if (argc >= 2) host = argv[1];
	if (argc >= 3) port = (unsigned short)atoi(argv[2]);
	if (argc >= 4) nick = argv[3];
	if (argc >= 5) room = argv[4];

	if (connect_host(host, port) != 0) return 1;

	g_stop = false;
	pthread_t th;
	if (pthread_create(&th, NULL, reader_main, NULL) != 0) {
		fputs("pthread_create failed\n", stderr);
		close(g_sock);
		return 1;
	}

	if (nick && room) {
		char b[MIXNET_MAX_LINE];
		snprintf(b, sizeof b, "%s %s", MX_HELLO, nick);
		if (send_line(b) != 0) goto done;
		snprintf(b, sizeof b, "%s %s", MX_JOIN, room);
		if (send_line(b) != 0) goto done;
	}

	fputs("mixnet (Amiga) — type protocol lines or " MX_CMD_QUIT "\n", stdout);
	for (;;) {
		char line[MIXNET_MAX_LINE + 2];
		if (!fgets(line, sizeof line, stdin)) break;
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = '\0';
		if (strcmp(line, MX_CMD_QUIT) == 0) {
			send_line(MX_QUIT);
			break;
		}
		if (len >= (size_t)MIXNET_MAX_LINE - 1u) {
			fputs("line too long\n", stderr);
			continue;
		}
		if (send_line(line) != 0) break;
	}
done:
	g_stop = true;
	if (g_sock >= 0) {
		shutdown(g_sock, 2);
		close(g_sock);
		g_sock = -1;
	}
	pthread_join(th, NULL);
	return 0;
}
