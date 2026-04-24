/*
 * 68mixCross console client for Windows 95/98/NT (Winsock 1.1).
 * Build (VC++ 5/6):  cl /W3 /Fe mixnet.exe mixnet.c /link wsock32.lib
 * MinGW:  gcc -O2 -o mixnet.exe mixnet.c -lwsock32
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include "../include/mixnet_config.h"

static volatile LONG gStopReader;
static SOCKET gSock = INVALID_SOCKET;

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
		fputs("socket() failed\n", stderr);
		return -1;
	}
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
				fputs("gethostbyname failed\n", stderr);
				closesocket(gSock);
				gSock = INVALID_SOCKET;
				WSACleanup();
				return -1;
			}
			memcpy(&addr.sin_addr, he->h_addr, (size_t)he->h_length);
		}
	}
	if (connect(gSock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		fputs("connect failed\n", stderr);
		closesocket(gSock);
		gSock = INVALID_SOCKET;
		WSACleanup();
		return -1;
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
	int sent = 0;
	while (sent < (int)n) {
		int w = send(gSock, buf + (size_t)sent, (int)((size_t)n - (size_t)sent), 0);
		if (w <= 0) return -1;
		sent += w;
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

	InterlockedExchange(&gStopReader, 0);
	uintptr_t th = _beginthreadex(NULL, 0, reader_main, NULL, 0, NULL);
	if (th == 0) {
		fputs("thread create failed\n", stderr);
		closesocket(gSock);
		WSACleanup();
		return 1;
	}

	if (nick && room) {
		char b[MIXNET_MAX_LINE];
		sprintf(b, "HELLO %s", nick);
		if (send_line(b) != 0) goto done;
		sprintf(b, "JOIN %s", room);
		if (send_line(b) != 0) goto done;
	}

	fputs("Enter protocol lines (HELLO, JOIN, MSG, PART, WHO, ROOMS, PING, QUIT) or :quit\n", stdout);
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
	InterlockedExchange(&gStopReader, 1);
	if (gSock != INVALID_SOCKET) {
		shutdown(gSock, 2);
		closesocket(gSock);
		gSock = INVALID_SOCKET;
	}
	WaitForSingleObject((HANDLE)th, 5000);
	CloseHandle((HANDLE)th);
	WSACleanup();
	return 0;
}
