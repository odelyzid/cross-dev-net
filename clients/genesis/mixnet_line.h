/* Line framing for mixnet (v0) over an 8-bit byte stream (UART, USB-serial, bridge). */
#ifndef MIXNET_LINE_H
#define MIXNET_LINE_H

#include "../include/mixnet_config.h"
#include <stddef.h>

typedef struct mixnet_line_rx {
	unsigned char buf[MIXNET_MAX_LINE + 2];
	int len;
} mixnet_line_rx_t;

void mixnet_line_rx_init(mixnet_line_rx_t* s);

/* Feed one received byte. Returns 1 if out_line is a full null-terminated line; 0 if more data needed. */
int mixnet_line_rx_byte(mixnet_line_rx_t* s, int byte_in, char* out_line, size_t out_cap);

/* Write one protocol line: appends \n, calls tx(user, byte) for each byte. Returns 0 on success. */
typedef void (*mixnet_tx_fn)(void* user, int byte);
int mixnet_write_line(const char* line_without_lf, mixnet_tx_fn tx, void* user);

#endif
