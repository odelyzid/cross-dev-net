/*
 * mixnet_ps2_link: transport <-> navigator glue (see header).
 */
#include "mixnet_ps2_link.h"
#include "../psx/mixnet_navigator.h"
#include "../include/mixnet_config.h"
#include "../common/mixnet_line.h"
#include "../common/mixnet_packet.h"

static const struct mixnet_ps2_transport* s_tr;

void mixnet_ps2_link_bind(const struct mixnet_ps2_transport* tr) {
	s_tr = tr;
}

void mixnet_ps2_link_tx(void* user, int byte) {
	(void)user;
	if (s_tr) s_tr->tx(byte);
}

void mixnet_ps2_link_feed(int byte) {
	static mixnet_line_rx_t s_line_rx;
	static mixnet_pkt_rx_t s_pkt_rx;
	static int s_mode;   /* 0=detect, 1=packet, 2=text */
	char lbuf[MIXNET_MAX_LINE + 4];

	if (s_mode == 0) {
		s_mode = (byte == PKT_MAGIC) ? 1 : 2;
		if (s_mode == 1)
			mixnet_pkt_rx_init(&s_pkt_rx);
	}

	if (s_mode == 1) {
		mixnet_pkt_t pkt;
		int r = mixnet_pkt_rx_byte(&s_pkt_rx, byte, &pkt);
		if (r == 1) {
			char txt[MIXNET_MAX_LINE + 4];
			if (mixnet_pkt_to_text(&pkt, txt, sizeof txt) == 0)
				(void)mixnet_nav_on_incoming_line(txt);
		} else if (r < 0) {
			mixnet_pkt_rx_init(&s_pkt_rx);
		}
	} else {
		if (mixnet_line_rx_byte(&s_line_rx, byte, lbuf, (size_t)sizeof lbuf))
			(void)mixnet_nav_on_incoming_line(lbuf);
	}
}