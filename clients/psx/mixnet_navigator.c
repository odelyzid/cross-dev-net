/* PSX "Mixnet Navigator" — Netscape-style text hub for mixnetd (v0). C89. */
#include "mixnet_navigator.h"
#include "../include/mixnet_proto.h"
#include <stddef.h>
#include <string.h>

#define NAV_URL_CAP 200

static mixnet_tx_fn s_tx;
static void* s_tx_user;
static int s_want_quit;

static char s_log[MIXNET_PSX_SCROLL][MIXNET_MAX_LINE];
static int s_log_n;

static char s_host[64];
static int s_port;
static char s_room[36];
static char s_nick[36];
static int s_have_hello;
static int s_in_room;

static char s_status[120];
static char s_urlbuf[NAV_URL_CAP];

/* ---- string helpers -------------------------------------------------- */

static void str_clear(char* s, size_t n) {
	while (n > 0) s[--n] = '\0';
}

static void str_copy_lim(char* dst, const char* src, size_t cap) {
	size_t i;
	if (cap == 0) return;
	for (i = 0; i < cap - 1 && src[i]; i++) dst[i] = src[i];
	dst[i] = '\0';
}

static void status_set(const char* m) { str_copy_lim(s_status, m ? m : "", sizeof s_status); }

static int strcasecmp_b(const char* a, const char* b) {
	for (; *a && *b; a++, b++) {
		unsigned char ca = (unsigned char)*a;
		unsigned char cb = (unsigned char)*b;
		if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca - (unsigned char)'A' + (unsigned char)'a');
		if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb - (unsigned char)'A' + (unsigned char)'a');
		if (ca != cb) return 1;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

static int append_char(char* buf, size_t cap, char c) {
	size_t n = strlen(buf);
	if (n + 1 >= cap) return -1;
	buf[n] = c;
	buf[n + 1] = '\0';
	return 0;
}

static void int_to_str(int v, char* out, size_t cap) {
	int tmp[8];
	int n = 0, i, vv;
	if (cap < 2) {
		if (cap == 1) out[0] = '\0';
		return;
	}
	if (v == 0) {
		out[0] = '0';
		out[1] = '\0';
		return;
	}
	if (v < 0) v = -v;
	for (vv = v; vv > 0 && n < 8; vv /= 10) tmp[n++] = vv % 10;
	for (i = 0; i < n && (size_t)i < cap - 1; i++) out[i] = (char)('0' + tmp[n - 1 - i]);
	out[i] = '\0';
}

static void build_url_string(void) {
	char u[NAV_URL_CAP];
	int val;
	char pstr[12];
	str_clear(u, sizeof u);
	str_copy_lim(u, "mixnet://", sizeof u);
	if (s_host[0]) strncat(u, s_host, sizeof u - strlen(u) - 1u);
	val = s_port;
	if (val <= 0) val = (int)MIXNET_DEFAULT_PORT;
	(void)append_char(u, sizeof u, ':');
	int_to_str(val, pstr, sizeof pstr);
	strncat(u, pstr, sizeof u - strlen(u) - 1u);
	if (s_room[0]) {
		(void)append_char(u, sizeof u, '/');
		strncat(u, s_room, sizeof u - strlen(u) - 1u);
	}
	str_copy_lim(s_urlbuf, u, sizeof s_urlbuf);
}

static void scroll_push(const char* line) {
	int j;
	if (!line) line = "";
	if (s_log_n < MIXNET_PSX_SCROLL) {
		str_copy_lim(s_log[s_log_n], line, (size_t)MIXNET_MAX_LINE);
		s_log_n++;
	} else {
		for (j = 0; j < MIXNET_PSX_SCROLL - 1; j++) {
			memcpy(s_log[j], s_log[j + 1], (size_t)MIXNET_MAX_LINE);
		}
		str_copy_lim(s_log[MIXNET_PSX_SCROLL - 1], line, (size_t)MIXNET_MAX_LINE);
	}
}

static int prefix_is(const char* s, const char* pre) { return strncmp(s, pre, strlen(pre)) == 0; }

/* mixnet://host:port/room */
static int parse_mixnet_url(const char* s, char* host, size_t hcap, int* port, char* room, size_t rcap) {
	const char* p;
	const char* slash;
	const char* cptr;
	if (hcap < 2 || rcap < 2) return -1;
	str_clear(host, hcap);
	str_clear(room, rcap);
	*port = (int)MIXNET_DEFAULT_PORT;
	if (strncmp(s, "mixnet://", 9) != 0) return -1;
	p = s + 9;
	slash = NULL;
	{
		const char* x;
		for (x = p; *x; x++) {
			if (*x == '/') {
				slash = x;
				break;
			}
		}
	}
	if (slash) {
		for (cptr = p; cptr < slash; cptr++) {
			if (*cptr == ':') {
				const char* q;
				int po = 0;
				{
					size_t hl = (size_t)(cptr - p);
					if (hl >= hcap) hl = hcap - 1;
					memcpy(host, p, hl);
					host[hl] = '\0';
				}
				for (q = cptr + 1; q < slash; q++) {
					if (*q < '0' || *q > '9') return -1;
					po = po * 10 + (int)(*q - '0');
					if (po > 65535) return -1;
				}
				*port = po;
				if (slash[1] && *(slash + 1)) str_copy_lim(room, slash + 1, rcap);
				return 0;
			}
		}
		{
			size_t hl = (size_t)(slash - p);
			if (hl >= hcap) hl = hcap - 1;
			memcpy(host, p, hl);
			host[hl] = '\0';
		}
		if (slash[1] && *(slash + 1)) str_copy_lim(room, slash + 1, rcap);
		return 0;
	}
	for (cptr = p; *cptr; cptr++) {
		if (*cptr == ':') {
			{
				size_t hl = (size_t)(cptr - p);
				if (hl >= hcap) hl = hcap - 1;
				memcpy(host, p, hl);
				host[hl] = '\0';
			}
			{
				int po = 0;
				for (cptr++; *cptr; cptr++) {
					if (*cptr < '0' || *cptr > '9') return -1;
					po = po * 10 + (int)(*cptr - '0');
					if (po > 65535) return -1;
				}
				*port = po;
			}
			return 0;
		}
	}
	str_copy_lim(host, p, hcap);
	return 0;
}

/* ---- server -> UI ---------------------------------------------------- */

static void on_ok_info_err(const char* line) {
	scroll_push(line);
	if (prefix_is(line, "OK ")) {
		if (prefix_is(line, "OK join ")) {
			str_copy_lim(s_room, line + 8, sizeof s_room);
			s_in_room = 1;
			status_set("in room");
		} else if (prefix_is(line, "OK part")) {
			s_in_room = 0;
			s_room[0] = '\0';
			status_set("left room");
		} else if (prefix_is(line, "OK hello ")) {
			s_have_hello = 1;
			status_set("hello ok");
		}
	} else if (prefix_is(line, "ERR ")) {
		status_set(line);
	} else if (strcmp(line, "PONG") == 0) {
		status_set("pong");
	}
}

void mixnet_nav_on_incoming_line(const char* line) {
	if (!line) return;
	if (prefix_is(line, "PRIVMSG ")) {
		scroll_push(line);
		return;
	}
	on_ok_info_err(line);
}

static void send_proto(const char* s, char* out, size_t out_cap) {
	if (s_tx) (void)mixnet_write_line(s, s_tx, s_tx_user);
	if (out && out_cap > 0) str_copy_lim(out, s, out_cap);
}

/* ---- public ---------------------------------------------------------- */

void mixnet_nav_init(mixnet_tx_fn link_tx, void* link_user) {
	int i;
	s_tx = link_tx;
	s_tx_user = link_user;
	s_want_quit = 0;
	s_log_n = 0;
	s_host[0] = '\0';
	s_port = (int)MIXNET_DEFAULT_PORT;
	s_room[0] = '\0';
	s_nick[0] = '\0';
	s_have_hello = 0;
	s_in_room = 0;
	for (i = 0; i < MIXNET_PSX_SCROLL; i++) s_log[i][0] = '\0';
	status_set("ready");
	s_urlbuf[0] = '\0';
	scroll_push("=== " MIXNET_NAV_TITLE " " MIXNET_NAV_VER " ===");
	scroll_push("Netscape-style hub. :h=help :q=exit :loc mixnet://host:port/room :g=connect");
	build_url_string();
}

void mixnet_nav_reset(void) { mixnet_nav_init(s_tx, s_tx_user); }

int mixnet_nav_want_quit(void) { return s_want_quit; }

int mixnet_nav_user_key(const char* user_line, char* out, size_t out_cap) {
	char buf[MIXNET_MAX_LINE + 8];
	if (!user_line) return 0;
	str_copy_lim(buf, user_line, sizeof buf);
	{
		size_t n = strlen(buf);
		while (n > 0 && (buf[n - 1] == ' ' || buf[n - 1] == '\r' || buf[n - 1] == '\n')) buf[--n] = '\0';
	}
	if (buf[0] == '\0') return 0;
	if (out && out_cap > 0) out[0] = '\0';

	if (strcmp(buf, ":q") == 0 || strcasecmp_b(buf, ":quit") == 0) {
		if (s_tx) (void)mixnet_write_line(MX_QUIT, s_tx, s_tx_user);
		s_want_quit = 1;
		return -1;
	}
	if (strcmp(buf, ":h") == 0 || strcasecmp_b(buf, ":help") == 0) {
		scroll_push("-- Help --");
		scroll_push("Location: :loc mixnet://host:port/room  (for display; PC bridge uses TCP to host:port)");
		scroll_push(":nick <name>  :g  (HELLO+JOIN)  :rooms  :who  :part  :m <msg>  :j <room>");
		scroll_push("Keys 1-6: hub menu. Or type raw HELLO, JOIN, MSG t, PING, ...");
		return 0;
	}
	if (prefix_is(buf, ":loc ")) {
		if (parse_mixnet_url(buf + 5, s_host, sizeof s_host, &s_port, s_room, sizeof s_room) != 0) {
			status_set("bad url");
			return 0;
		}
		build_url_string();
		scroll_push(s_urlbuf);
		status_set("location set");
		return 0;
	}
	if (prefix_is(buf, ":nick ")) {
		const char* n = buf + 6;
		while (*n == ' ') n++;
		str_copy_lim(s_nick, n, sizeof s_nick);
		scroll_push("(nick set)");
		return 0;
	}
	if (strcmp(buf, ":g") == 0 || strcasecmp_b(buf, ":go") == 0) {
		char tmp[128];
		if (!s_nick[0]) str_copy_lim(s_nick, "psx", sizeof s_nick);
		str_copy_lim(tmp, MX_HELLO, sizeof tmp);
		strncat(tmp, " ", sizeof tmp - strlen(tmp) - 1u);
		strncat(tmp, s_nick, sizeof tmp - strlen(tmp) - 1u);
		send_proto(tmp, out, out_cap);
		if (s_room[0]) {
			str_copy_lim(tmp, MX_JOIN, sizeof tmp);
			strncat(tmp, " ", sizeof tmp - strlen(tmp) - 1u);
			strncat(tmp, s_room, sizeof tmp - strlen(tmp) - 1u);
			send_proto(tmp, out, out_cap);
		}
		return 0;
	}
	if (strcmp(buf, ":rooms") == 0) { send_proto("ROOMS", out, out_cap); return 0; }
	if (strcmp(buf, ":who") == 0)  { send_proto("WHO", out, out_cap); return 0; }
	if (strcmp(buf, ":part") == 0) { send_proto("PART", out, out_cap); return 0; }
	if (strcmp(buf, ":ping") == 0) { send_proto("PING", out, out_cap); return 0; }
	if (prefix_is(buf, ":j ")) {
		const char* r = buf + 3;
		while (*r == ' ') r++;
		{
			char t[MIXNET_MAX_LINE];
			str_copy_lim(t, MX_JOIN, sizeof t);
			strncat(t, " ", sizeof t - strlen(t) - 1u);
			strncat(t, r, sizeof t - strlen(t) - 1u);
			str_copy_lim(s_room, r, sizeof s_room);
			send_proto(t, out, out_cap);
		}
		return 0;
	}
	if (prefix_is(buf, ":m ")) {
		const char* t = buf + 3;
		char line[MIXNET_MAX_LINE];
		while (*t == ' ') t++;
		if (!s_have_hello || !s_in_room) {
			status_set("ERR need joined room");
			return 0;
		}
		str_copy_lim(line, MX_MSG, sizeof line);
		strncat(line, " ", sizeof line - strlen(line) - 1u);
		strncat(line, t, sizeof line - strlen(line) - 1u);
		send_proto(line, out, out_cap);
		return 0;
	}
	if (prefix_is(buf, "HELLO ") || prefix_is(buf, "JOIN ") || prefix_is(buf, "PART")
	    || prefix_is(buf, "MSG ") || (strcmp(buf, "ROOMS") == 0) || (strcmp(buf, "WHO") == 0)
	    || (strcmp(buf, "PING") == 0) || (strcmp(buf, "QUIT") == 0)) {
		send_proto(buf, out, out_cap);
		return 0;
	}
	if (buf[0] == '1' && buf[1] == '\0') { scroll_push("[1] :loc url  then  :g"); return 0; }
	if (buf[0] == '2' && buf[1] == '\0') { send_proto("ROOMS", out, out_cap); return 0; }
	if (buf[0] == '3' && buf[1] == '\0') { send_proto("WHO", out, out_cap); return 0; }
	if (buf[0] == '4' && buf[1] == '\0') { send_proto("PART", out, out_cap); return 0; }
	if (buf[0] == '5' && buf[1] == '\0') { scroll_push("Use :m <text> to MSG"); return 0; }
	if (buf[0] == '6' && buf[1] == '\0') { scroll_push("Raw: HELLO nick, JOIN r, PING, ..."); return 0; }
	scroll_push("Unknown. :h");
	return 0;
}

int mixnet_nav_render_screen(char* out, size_t cap) {
	size_t used = 0;
	int i;
	const char* bar = "---- ";
	const char* x;
	if (!out || cap < 2) return 0;
	#define WSTR(s) do { for (x = (s); *x && used < cap - 1; x++) out[used++] = *x; } while (0)
	WSTR(MIXNET_NAV_TITLE " | "); WSTR(MIXNET_NAV_VER); WSTR("\n");
	WSTR("File  View  Go  Services  Help   (keys 1-6)\n");
	WSTR("Location: ");
	WSTR(s_urlbuf[0] ? s_urlbuf : "mixnet://(set :loc)"); WSTR("\n");
	WSTR(bar); WSTR("Status: "); WSTR(s_status); WSTR("\n");
	for (i = 0; i < s_log_n && used < cap - 2; i++) {
		WSTR(s_log[i]);
		WSTR("\n");
	}
	out[used] = '\0';
	if (used >= cap) out[cap - 1] = '\0';
	#undef WSTR
	return (int)used;
}
