/*
 * POSIX test / dev client (Linux, macOS, AmigaOS with ixemul + BSD stack, etc.).
 * Build:  cc -O2 -pthread -o mixnet mixnet.c
 * Amiga (example): m68k-amigaos-gcc -o mixnet mixnet.c -lpthread -lsocket (stack-specific)
 */
#define _POSIX_C_SOURCE 200809L
#include "../include/mixnet_config.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int g_sock = -1;
static bool g_stop;

static void* reader_main(void* arg) {
	(void)arg;
	char buf[MIXNET_MAX_LINE + 4];
	int pos = 0;
	for (;;) {
		if (g_stop) break;
		unsigned char b;
		ssize_t n = recv(g_sock, &b, 1, 0);
		if (n <= 0) break;
		if (b == '\r') continue;
		if (b == '\n') {
			buf[pos] = '\0';
			puts(buf);
			fflush(stdout);
			pos = 0;
			continue;
		}
		if (pos < (int)sizeof(buf) - 2)
			buf[pos++] = (char)b;
		else
			pos = 0;
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
		snprintf(b, sizeof b, "HELLO %s", nick);
		if (send_line(b) != 0) goto done;
		snprintf(b, sizeof b, "JOIN %s", room);
		if (send_line(b) != 0) goto done;
	}

	fputs("Protocol lines (HELLO, JOIN, MSG, …) or :quit\n", stdout);
	for (;;) {
		char line[MIXNET_MAX_LINE + 2];
		if (!fgets(line, sizeof line, stdin)) break;
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = '\0';
		if (strcmp(line, ":quit") == 0) {
			send_line("QUIT");
			break;
		}
		if (len >= (size_t)MIXNET_MAX_LINE - 1u) {
			fputs("Line too long\n", stderr);
			continue;
		}
		if (send_line(line) != 0) break;
	}
done:
	g_stop = true;
	if (g_sock >= 0) {
		shutdown(g_sock, SHUT_RDWR);
		close(g_sock);
		g_sock = -1;
	}
	pthread_join(th, NULL);
	return 0;
}
