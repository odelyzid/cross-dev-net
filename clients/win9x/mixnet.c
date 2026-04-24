/*
 * 68mixCross console client for Windows 95/98/NT (Winsock 1.1).
 * Build (VC++ 5/6):  cl /W3 /Fe mixnet.exe mixnet.c /link wsock32.lib
 * MinGW:  gcc -std=c99 -O2 -o mixnet.exe mixnet.c -lwsock32
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include "../include/mixnet_config.h"
#include "../include/mixnet_proto.h"

#define MIXNET_W32_TITLE "68mix / mixnet"

static volatile LONG gStopReader;
static SOCKET gSock = INVALID_SOCKET;

static void wsa_print(const char* ctx) {
	int e = (int)WSAGetLastError();
	if (ctx && ctx[0])
		fprintf(stderr, "%s: winsock error %d\n", ctx, e);
	else
		fprintf(stderr, "winsock error %d\n", e);
}

static int is_help_arg(const char* a) {
	if (!a) return 0;
	if (strcmp(a, "-h") == 0) return 1;
	if (strcmp(a, "/?") == 0) return 1;
	if (strcmp(a, "--help") == 0) return 1;
	return 0;
}

static void print_usage(const char* argv0) {
	const char* base = argv0 && argv0[0] ? argv0 : "mixnet";
	fprintf(stderr,
		"Usage: %s [host] [port] [nick] [room]\n"
		"  default: host 127.0.0.1, port %u\n"
		"  With nick and room: send \"%s <nick>\" then \"%s <room>\" on connect.\n"
		"  Then type server lines (e.g. \"%s text\", %s) or " MX_CMD_QUIT " to send \"%s\" and exit.\n",
		base, (unsigned)MIXNET_DEFAULT_PORT, MX_HELLO, MX_JOIN, MX_MSG, MX_PING, MX_QUIT);
}

static unsigned __stdcall reader_main(void* arg) {
	(void)arg;
	char buf[MIXNET_MAX_LINE + 4];
	int pos = 0;
	for (;;) {
		if (InterlockedCompareExchange(&gStopReader, 0, 0) != 0) break;
		unsigned char b;
		int n = recv(gSock, (char*)&b, 1, 0);
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
	return 0;
}

static int connect_host(const char* host, unsigned short port) {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(1, 1), &wsa) != 0) {
		fputs("WSAStartup failed\n", stderr);
		return -1;
	}
	gSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (gSock == INVALID_SOCKET) {
		wsa_print("socket");
		WSACleanup();
		return -1;
	}
	{
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		{
			unsigned long a = inet_addr(host);
			if (a != INADDR_NONE) {
				addr.sin_addr.s_addr = a;
			} else {
				struct hostent* he = gethostbyname(host);
				if (!he || !he->h_addr) {
					fprintf(stderr, "gethostbyname: %s\n", host);
					closesocket(gSock);
					gSock = INVALID_SOCKET;
					WSACleanup();
					return -1;
				}
				memcpy(&addr.sin_addr, he->h_addr, (size_t)he->h_length);
			}
		}
		if (connect(gSock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
			wsa_print("connect");
			closesocket(gSock);
			gSock = INVALID_SOCKET;
			WSACleanup();
			return -1;
		}
	}
	return 0;
}

static int send_line(const char* s) {
	char buf[MIXNET_MAX_LINE + 2];
	size_t n = strlen(s);
	if (n > MIXNET_MAX_LINE) return -1;
	memcpy(buf, s, n);
	if (n == 0 || s[n - 1] != '\n')
		buf[n++] = '\n';
	{
		int sent = 0;
		while (sent < (int)n) {
			int w = send(gSock, buf + (size_t)sent, (int)((size_t)n - (size_t)sent), 0);
			if (w <= 0) {
				wsa_print("send");
				return -1;
			}
			sent += w;
		}
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* host = "127.0.0.1";
	unsigned short port = (unsigned short)MIXNET_DEFAULT_PORT;
	const char* nick = NULL;
	const char* room = NULL;
	int argi = 1;

	if (argc >= 2 && is_help_arg(argv[1])) {
		print_usage(argv[0] ? argv[0] : "mixnet");
		return 0;
	}

	if (argi < argc) host = argv[argi++];
	if (argi < argc) port = (unsigned short)atoi(argv[argi++]);
	if (argi < argc) nick = argv[argi++];
	if (argi < argc) room = argv[argi++];

	SetConsoleTitleA(MIXNET_W32_TITLE);

	if (connect_host(host, port) != 0) return 1;

	InterlockedExchange(&gStopReader, 0);
	{
		uintptr_t th = _beginthreadex(NULL, 0, reader_main, NULL, 0, NULL);
		if (th == 0) {
			fputs("thread create failed\n", stderr);
			closesocket(gSock);
			WSACleanup();
			return 1;
		}
		if (nick && room) {
			char b[MIXNET_MAX_LINE];
			sprintf(b, "%s %s", MX_HELLO, nick);
			if (send_line(b) != 0) goto done;
			sprintf(b, "%s %s", MX_JOIN, room);
			if (send_line(b) != 0) goto done;
		}
		fputs("Connected. Server lines print below. Type " MX_CMD_QUIT " to send " MX_QUIT " and exit.\n", stdout);
		for (;;) {
			char line[MIXNET_MAX_LINE + 2];
			size_t len;
			if (!fgets(line, sizeof line, stdin)) break;
			len = strlen(line);
			while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
				line[--len] = '\0';
			if (strcmp(line, MX_CMD_QUIT) == 0) {
				send_line(MX_QUIT);
				break;
			}
			if (len >= (size_t)MIXNET_MAX_LINE - 1u) {
				fputs("Line too long\n", stderr);
				continue;
			}
			if (send_line(line) != 0) break;
		}
done:
		InterlockedExchange(&gStopReader, 1);
		if (gSock != INVALID_SOCKET) {
			shutdown(gSock, 2);
			closesocket(gSock);
			gSock = INVALID_SOCKET;
		}
		WaitForSingleObject((HANDLE)th, 5000);
		CloseHandle((HANDLE)th);
		WSACleanup();
	}
	return 0;
}
