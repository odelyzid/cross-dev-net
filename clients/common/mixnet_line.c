#include "mixnet_line.h"
#include <string.h>

void mixnet_line_rx_init(mixnet_line_rx_t* s) {
	s->len = 0;
}

int mixnet_line_rx_byte(mixnet_line_rx_t* s, int byte_in, char* out_line, size_t out_cap) {
	unsigned char b;
	b = (unsigned char)byte_in;
	if (b == '\r') return 0;
	if (b == '\n') {
		if (s->len <= 0) return 0;
		if ((size_t)s->len >= out_cap) {
			s->len = 0;
			return 0;
		}
		s->buf[s->len] = '\0';
		if (out_line) {
			(void)memcpy(out_line, s->buf, (size_t)s->len + 1u);
		}
		s->len = 0;
		return 1;
	}
	if (s->len < (int)sizeof(s->buf) - 2) {
		s->buf[s->len++] = b;
	} else {
		s->len = 0;
	}
	return 0;
}

int mixnet_write_line(const char* line, mixnet_tx_fn tx, void* user) {
	size_t n;
	size_t i;
	if (!line || !tx) return -1;
	n = 0;
	while (line[n] && n < (size_t)MIXNET_MAX_LINE) n++;
	if (n > MIXNET_MAX_LINE) return -1;
	for (i = 0; i < n; i++) tx(user, (int)(unsigned char)line[i]);
	if (n == 0u || line[n - 1] != '\n') tx(user, (int)'\n');
	return 0;
}
