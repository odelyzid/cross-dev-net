/* Mega Drive Port 2 serial driver — hardware I/O + ring buffer + EXT ISR.
 * Reference: rhargreaves/mega-drive-serial-port serial.c */
#include "serial.h"

/* ------------------------------------------------------------------ */
/*  Ring buffer (ISR-safe, single-producer single-consumer)           */
/* ------------------------------------------------------------------ */
static u8 rx_buf[SERIAL_RX_BUF_SIZE];
static volatile u16 rx_head;
static volatile u16 rx_tail;

#define RX_BUF_MASK (SERIAL_RX_BUF_SIZE - 1)

/* ------------------------------------------------------------------ */
/*  EXT interrupt handler — called when a byte arrives on Port 2 RX   */
/* ------------------------------------------------------------------ */
static void ext_int_handler(void)
{
	u8 byte;
	u16 next;

	byte = *(vu8 *)PORT2_RX;
	next = (rx_head + 1) & RX_BUF_MASK;
	if (next != rx_tail) {
		rx_buf[rx_head] = byte;
		rx_head = next;
	}
	/* clear error flag so the next interrupt can fire */
	*(vu8 *)PORT2_SCTRL &= ~SCTRL_RERR;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void serial_init(u8 sctrl_flags)
{
	/* write SCTRL first — enables the serial peripheral */
	*(vu8 *)PORT2_SCTRL = sctrl_flags;
	/* set TH pin as output (serial TX on pin 6) */
	*(vu8 *)PORT2_CTRL = CTRL_PCS_OUT;

	rx_head = 0;
	rx_tail = 0;

	if (sctrl_flags & SCTRL_RINT) {
		/* Enable External Interrupt 2 on the VDP */
		VDP_setReg(11, 0x08);
		SYS_setExtIntCallback(&ext_int_handler);
		SYS_setInterruptMaskLevel(1);
	}
}

u8 serial_sctrl(void)
{
	return *(vu8 *)PORT2_SCTRL;
}

bool serial_ready_to_receive(void)
{
	return (*(vu8 *)PORT2_SCTRL & SCTRL_RRDY) != 0;
}

u8 serial_receive(void)
{
	return *(vu8 *)PORT2_RX;
}

void serial_send(u8 data)
{
	*(vu8 *)PORT2_TX = data;
}

bool serial_ready_to_send(void)
{
	return (*(vu8 *)PORT2_SCTRL & SCTRL_TFUL) == 0;
}

void serial_send_when_ready(u8 data)
{
	while (!serial_ready_to_send())
		;
	serial_send(data);
}

int serial_drain(serial_rx_byte_fn cb, void *user)
{
	int count = 0;
	while (rx_tail != rx_head) {
		u8 byte = rx_buf[rx_tail];
		rx_tail = (rx_tail + 1) & RX_BUF_MASK;
		if (cb)
			cb(byte, user);
		count++;
	}
	return count;
}
