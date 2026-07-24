#include "mixnet_packet.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  RX state machine                                                   */
/* ------------------------------------------------------------------ */

void mixnet_pkt_rx_init(mixnet_pkt_rx_t* s) {
	s->stage = 0;
	s->remaining = 0;
	s->pkt_len = 0;
}

int mixnet_pkt_rx_byte(mixnet_pkt_rx_t* s, int byte_in, mixnet_pkt_t* out_pkt) {
	unsigned char b = (unsigned char)byte_in;

	switch (s->stage) {
	case 0: /* SYNC — wait for magic byte 0x58 */
		if (b == PKT_MAGIC) {
			out_pkt->hdr[0] = b;
			s->stage = 1;
		}
		return 0;

	case 1: /* TYPE */
		out_pkt->hdr[1] = b;
		s->stage = 2;
		return 0;

	case 2: /* FLAGS */
		out_pkt->hdr[2] = b;
		s->stage = 3;
		return 0;

	case 3: /* LENGTH high byte */
		out_pkt->hdr[3] = b;
		s->pkt_len = (unsigned short)b << 8;
		s->stage = 4;
		return 0;

	case 4: /* LENGTH low byte */
		out_pkt->hdr[4] = b;
		s->pkt_len |= (unsigned short)b;
		out_pkt->payload_len = 0;
		if (s->pkt_len == 0) {
			s->stage = 0;
			return 1; /* empty payload */
		}
		if (s->pkt_len > (unsigned short)MIXNET_MAX_LINE) {
			/* protocol error — reset */
			mixnet_pkt_rx_init(s);
			return -1;
		}
		s->remaining = (unsigned int)s->pkt_len;
		s->stage = 5;
		return 0;

	case 5: /* PAYLOAD */
		if (s->remaining > 0) {
			out_pkt->payload[out_pkt->payload_len++] = b;
			s->remaining--;
		}
		if (s->remaining == 0) {
			s->stage = 0;
			return 1;
		}
		return 0;

	default:
		mixnet_pkt_rx_init(s);
		return -1;
	}
}

/* ------------------------------------------------------------------ */
/*  TX — serialize via callback                                        */
/* ------------------------------------------------------------------ */

int mixnet_pkt_send(const mixnet_pkt_t* pkt, mixnet_tx_fn tx, void* user) {
	int i;
	if (!pkt || !tx) return -1;
	for (i = 0; i < PKT_HEADER_SIZE; i++)
		tx(user, (int)pkt->hdr[i]);
	for (i = 0; i < pkt->payload_len; i++)
		tx(user, (int)pkt->payload[i]);
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Build helpers                                                      */
/* ------------------------------------------------------------------ */

void mixnet_pkt_start(mixnet_pkt_t* pkt, int type, int flags) {
	pkt->hdr[0] = PKT_MAGIC;
	pkt->hdr[1] = (unsigned char)(type & 0xFF);
	pkt->hdr[2] = (unsigned char)(flags & 0xFF);
	pkt->hdr[3] = 0;
	pkt->hdr[4] = 0;
	pkt->payload_len = 0;
}

int mixnet_pkt_append(mixnet_pkt_t* pkt, const void* data, int len) {
	int new_len;
	if (!pkt || !data) return -1;
	if (len < 0) return -1;
	new_len = pkt->payload_len + len;
	if (new_len > (int)MIXNET_MAX_LINE) return -1;
	memcpy(pkt->payload + pkt->payload_len, data, (size_t)len);
	pkt->payload_len = new_len;
	pkt->hdr[3] = (unsigned char)((new_len >> 8) & 0xFF);
	pkt->hdr[4] = (unsigned char)(new_len & 0xFF);
	return 0;
}

/* ------------------------------------------------------------------ */
/*  Text <-> packet conversion                                         */
/* ------------------------------------------------------------------ */

static const struct {
	int type;
	const char* prefix;
} s_type_map[] = {
	{PKT_HELLO,      "HELLO"},
	{PKT_HELLO_OK,   "OK hello"},
	{PKT_ERR,        "ERR"},
	{PKT_JOIN,       "JOIN"},
	{PKT_JOIN_OK,    "OK join"},
	{PKT_MSG,        "MSG"},
	{PKT_PRIVMSG,    "PRIVMSG"},
	{PKT_PING,       "PING"},
	{PKT_PONG,       "PONG"},
	{PKT_QUIT,       "QUIT"},
	{PKT_QUIT_OK,    "OK bye"},
	{PKT_PART,       "PART"},
	{PKT_PART_OK,    "OK part"},
	{PKT_WHO,        "WHO"},
	{PKT_WHO_RESP,   "OK who"},
	{PKT_ROOMS,      "ROOMS"},
	{PKT_ROOMS_RESP, "OK rooms"},
	{PKT_INFO,       "INFO"},
};

#define S_TYPE_MAP_COUNT ((int)(sizeof(s_type_map) / sizeof(s_type_map[0])))

static const char* type_to_prefix(int type) {
	int i;
	for (i = 0; i < S_TYPE_MAP_COUNT; i++) {
		if (s_type_map[i].type == type)
			return s_type_map[i].prefix;
	}
	return NULL;
}

int mixnet_pkt_to_text(const mixnet_pkt_t* pkt, char* out, size_t cap) {
	const char* prefix;
	size_t plen, used = 0, i;
	if (!pkt || !out || cap < 2) return -1;
	prefix = type_to_prefix((int)pkt->hdr[1]);
	if (!prefix) return -1;
	plen = strlen(prefix);
	if (plen >= cap) return -1;
	memcpy(out, prefix, plen);
	used = plen;
	if (pkt->payload_len > 0) {
		if (used + 1 >= cap) return -1;
		out[used++] = ' ';
		for (i = 0; i < (size_t)pkt->payload_len && used < cap - 1; i++) {
			unsigned char c = pkt->payload[i];
			if (c >= 0x20 && c <= 0x7E)
				out[used++] = (char)c;
			else if (c == '\0')
				break; /* null terminator in payload ends the string */
			else
				out[used++] = '.'; /* replace non-printable */
		}
	}
	out[used] = '\0';
	return 0;
}

int mixnet_text_to_pkt(const char* line, mixnet_pkt_t* out_pkt) {
	int i, rest_len;
	const char* rest;
	if (!line || !out_pkt) return -1;
	for (i = 0; i < S_TYPE_MAP_COUNT; i++) {
		const char* prefix = s_type_map[i].prefix;
		size_t plen = strlen(prefix);
		if (strncmp(line, prefix, plen) != 0)
			continue;
		if (line[plen] != '\0' && line[plen] != ' ')
			continue;
		mixnet_pkt_start(out_pkt, s_type_map[i].type, 0);
		rest = line + plen;
		while (*rest == ' ') rest++;
		rest_len = (int)strlen(rest);
		if (rest_len > 0) {
			if (mixnet_pkt_append(out_pkt, rest, rest_len) != 0)
				return -1;
		}
		return 0;
	}
	return -1; /* no matching prefix */
}

/* ------------------------------------------------------------------ */
/*  Self-test                                                          */
/* ------------------------------------------------------------------ */

static unsigned char s_test_buf[4096];
static int s_test_pos;

static void test_tx(void* user, int byte) {
	(void)user;
	if (s_test_pos < (int)sizeof(s_test_buf) - 1)
		s_test_buf[s_test_pos++] = (unsigned char)byte;
}

static int test_roundtrip(int type, int flags, const void* payload, int payload_len) {
	mixnet_pkt_t tx_pkt, rx_pkt;
	mixnet_pkt_rx_t rx_state;
	int i, result;
	const char* s;

	/* Build and serialize */
	mixnet_pkt_start(&tx_pkt, type, flags);
	if (payload && payload_len > 0) {
		if (mixnet_pkt_append(&tx_pkt, payload, payload_len) != 0) return 0;
	}
	s_test_pos = 0;
	if (mixnet_pkt_send(&tx_pkt, test_tx, NULL) != 0) return 0;

	/* Feed serialized bytes back through RX */
	mixnet_pkt_rx_init(&rx_state);
	result = 0;
	for (i = 0; i < s_test_pos; i++) {
		int r = mixnet_pkt_rx_byte(&rx_state, (int)s_test_buf[i], &rx_pkt);
		if (r == 1) { result = 1; break; }
		if (r < 0) return 0;
	}
	if (!result) return 0;

	/* Verify header fields match */
	if (rx_pkt.hdr[0] != PKT_MAGIC) return 0;
	if (rx_pkt.hdr[1] != (unsigned char)(type & 0xFF)) return 0;
	if (rx_pkt.hdr[2] != (unsigned char)(flags & 0xFF)) return 0;
	if (rx_pkt.payload_len != payload_len) return 0;
	if (payload_len > 0 && memcmp(rx_pkt.payload, payload, (size_t)payload_len) != 0) return 0;

	/* Convert to text and back */
	if (type >= PKT_APP_START) return 1; /* app types have no text mapping */
	s = type_to_prefix(type);
	if (!s) return 0;
	{
		char txt_buf[MIXNET_MAX_LINE + 4];
		mixnet_pkt_t back_pkt;
		if (mixnet_pkt_to_text(&rx_pkt, txt_buf, sizeof txt_buf) != 0) return 0;
		if (mixnet_text_to_pkt(txt_buf, &back_pkt) != 0) return 0;
		if (back_pkt.hdr[1] != rx_pkt.hdr[1]) return 0;
		if (back_pkt.payload_len != rx_pkt.payload_len) return 0;
		if (rx_pkt.payload_len > 0 &&
		    memcmp(back_pkt.payload, rx_pkt.payload, (size_t)rx_pkt.payload_len) != 0) return 0;
	}

	return 1;
}

static int test_v0_text_roundtrip(const char* line) {
	mixnet_pkt_t pkt;
	char out[MIXNET_MAX_LINE + 4];
	if (mixnet_text_to_pkt(line, &pkt) != 0) return 0;
	if (mixnet_pkt_to_text(&pkt, out, sizeof out) != 0) return 0;
	return (strcmp(line, out) == 0);
}

int mixnet_packet_selftest(void) {
	/* 1. Empty payload packets */
	if (!test_roundtrip(PKT_PING, 0, NULL, 0)) return 0;
	if (!test_roundtrip(PKT_PONG, 0, NULL, 0)) return 0;
	if (!test_roundtrip(PKT_QUIT, 0, NULL, 0)) return 0;

	/* 2. Packets with payload */
	if (!test_roundtrip(PKT_HELLO, 0, "alice", 5)) return 0;
	if (!test_roundtrip(PKT_JOIN, 0, "lobby", 5)) return 0;
	if (!test_roundtrip(PKT_MSG, 0, "hello world", 11)) return 0;
	if (!test_roundtrip(PKT_PRIVMSG, 0, "bob hey", 7)) return 0;
	if (!test_roundtrip(PKT_INFO, 0, "welcome", 7)) return 0;

	/* 3. With flags */
	if (!test_roundtrip(PKT_HELLO_OK, PKT_FLAG_ACKREQ, "0001", 4)) return 0;

	/* 4. v0 ASCII text round-trips */
	if (!test_v0_text_roundtrip("PING")) return 0;
	if (!test_v0_text_roundtrip("HELLO alice")) return 0;
	if (!test_v0_text_roundtrip("JOIN lobby")) return 0;
	if (!test_v0_text_roundtrip("MSG hello world")) return 0;
	if (!test_v0_text_roundtrip("OK hello 0001")) return 0;
	if (!test_v0_text_roundtrip("INFO welcome")) return 0;

	/* 5. RX error: payload exceeds limit */
	{
		mixnet_pkt_t big;
		mixnet_pkt_rx_t rx;
		mixnet_pkt_start(&big, PKT_MSG, 0);
		/* simulate oversized payload by writing header bytes directly */
		big.hdr[3] = 0x02; /* length high */
		big.hdr[4] = 0x00; /* length low = 512 */
		s_test_pos = 0;
		mixnet_pkt_send(&big, test_tx, NULL);
		mixnet_pkt_rx_init(&rx);
		{
			int i;
			for (i = 0; i < s_test_pos; i++) {
				int r = mixnet_pkt_rx_byte(&rx, (int)s_test_buf[i], &big);
				if (r < 0) break; /* expected error */
				if (r == 1) return 0; /* unexpected success */
			}
		}
	}

	return 1;
}
