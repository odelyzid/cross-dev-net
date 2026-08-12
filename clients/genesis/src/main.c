/* Genesis mixnet terminal — serial driver + terminal UI main loop. */
#include <genesis.h>
#include "main.h"
#include "serial.h"
#include "mixnet_line.h"
#include "serial_terminal.h"

/* serial → mixnet_line framing */
static mixnet_line_rx_t g_line_rx;
static char g_line_buf[MIXNET_MAX_LINE];

static void on_serial_byte(u8 byte, void *user)
{
	(void)user;
	if (mixnet_line_rx_byte(&g_line_rx, (int)byte, g_line_buf, sizeof(g_line_buf))) {
		terminal_push_line(g_line_buf);
	}
}

/* TX callback: send one byte via serial */
static void tx_byte(void *user, int byte)
{
	(void)user;
	serial_send_when_ready((u8)byte);
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
	terminal_on_joy(joy, changed, state);
}

int main(bool hardReset)
{
	(void)hardReset;

	SYS_disableInts();

	serial_init(SCTRL_4800_BPS | SCTRL_SIN | SCTRL_SOUT | SCTRL_RINT);
	mixnet_line_rx_init(&g_line_rx);
	terminal_init();

	VDP_setTextPalette(PAL0);

	SYS_enableInts();

	JOY_setEventHandler(&joyEvent);

	char tx_line[MIXNET_MAX_LINE + 4];

	while (TRUE) {
		/* Drain serial RX bytes into line framing */
		serial_drain(on_serial_byte, NULL);

		/* If a TX line is pending from the terminal, send it */
		if (terminal_poll_tx(tx_line, sizeof tx_line)) {
			mixnet_write_line(tx_line, tx_byte, NULL);
		}

		/* Render the terminal display */
		terminal_render();

		SYS_doVBlankProcess();
	}

	return 0;
}
