/* Binary packet framing for mixnet (v1) over any 8-bit byte stream.
 * Length-prefixed packets with 5-byte header: magic(1) + type(1) + flags(1) + length(2 big-endian).
 * Auto-detect: first byte 0x58 signals binary mode (otherwise v0 ASCII text).
 * Shared by all clients: TCP direct, serial bridge, UART.
 * See .plans/PACKET-PLAN.md */
#ifndef MIXNET_PACKET_H
#define MIXNET_PACKET_H

#include "../include/mixnet_config.h"
#include <stddef.h>

/* --- Packet types ------------------------------------------------------- */

typedef enum {
	PKT_HELLO      = 0x01,
	PKT_HELLO_OK   = 0x02,
	PKT_ERR        = 0x03,
	PKT_JOIN       = 0x04,
	PKT_JOIN_OK    = 0x05,
	PKT_MSG        = 0x06,
	PKT_PRIVMSG    = 0x07,
	PKT_PING       = 0x08,
	PKT_PONG       = 0x09,
	PKT_QUIT       = 0x0A,
	PKT_QUIT_OK    = 0x0B,
	PKT_PART       = 0x0C,
	PKT_PART_OK    = 0x0D,
	PKT_WHO        = 0x0E,
	PKT_WHO_RESP   = 0x0F,
	PKT_ROOMS      = 0x10,
	PKT_ROOMS_RESP = 0x11,
	PKT_INFO       = 0x12,
	PKT_APP_START  = 0x80   /* first type reserved for application/game use */
} mixnet_pkt_type_t;

/* --- Flags -------------------------------------------------------------- */

#define PKT_FLAG_ACKREQ  0x01   /* sender wants an ACK for this packet */
#define PKT_FLAG_ACKRESP 0x02   /* this packet is an ACK response */

/* --- Constants ----------------------------------------------------------- */

#define PKT_MAGIC       0x58    /* 'X' — binary mode discriminator */
#define PKT_HEADER_SIZE 5       /* magic + type + flags + 2-byte length */

/* --- RX state ----------------------------------------------------------- */

typedef struct {
	int stage;              /* 0=sync, 1=type, 2=flags, 3=len_hi, 4=len_lo, 5=payload */
	unsigned int remaining; /* payload bytes still needed */
	unsigned short pkt_len; /* decoded payload length */
} mixnet_pkt_rx_t;

/* --- Full packet (header + payload contiguous) -------------------------- */

typedef struct {
	unsigned char hdr[PKT_HEADER_SIZE];
	unsigned char payload[MIXNET_MAX_LINE];
	int payload_len;        /* actual number of bytes in payload[] */
} mixnet_pkt_t;

/* --- TX byte callback (matches mixnet_line.h signature) ----------------- */

typedef void (*mixnet_tx_fn)(void* user, int byte);

/* --- API ---------------------------------------------------------------- */

/* Reset RX state machine before feeding bytes. */
void mixnet_pkt_rx_init(mixnet_pkt_rx_t* s);

/* Feed one received byte. Returns:
 *   1  if out_pkt holds a complete packet (caller should process it)
 *   0  if more bytes are needed
 *  -1  on protocol error (payload exceeds MIXNET_MAX_LINE, etc.)
 */
int mixnet_pkt_rx_byte(mixnet_pkt_rx_t* s, int byte_in, mixnet_pkt_t* out_pkt);

/* Serialize a packet via tx callback (header + payload, no trailing \n).
 * Returns 0 on success, -1 on error. */
int mixnet_pkt_send(const mixnet_pkt_t* pkt, mixnet_tx_fn tx, void* user);

/* Build helpers: start a new packet, then append payload data.
 * mixnet_pkt_append updates the length field in hdr[] automatically. */
void mixnet_pkt_start(mixnet_pkt_t* pkt, int type, int flags);
int  mixnet_pkt_append(mixnet_pkt_t* pkt, const void* data, int len);

/* Convert between binary packet and v0 ASCII line (null-terminated).
 * Both functions return 0 on success, -1 on error (truncation, unknown type). */
int mixnet_pkt_to_text(const mixnet_pkt_t* pkt, char* out, size_t cap);
int mixnet_text_to_pkt(const char* line, mixnet_pkt_t* out_pkt);

/* Self-test: returns 1 on pass, 0 on fail. Call from any main(). */
int mixnet_packet_selftest(void);

#endif
