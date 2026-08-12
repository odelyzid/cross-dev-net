/* Mega Drive Port 2 serial driver — register defs + API.
 * Matches the hardware spec from rhargreaves/mega-drive-serial-port.
 * Wire into mixnet_line (v0 text) for protocol framing. */
#ifndef SERIAL_H
#define SERIAL_H

#include <genesis.h>

/* Port 2 hardware register addresses */
#define PORT2_CTRL   0xA1000B
#define PORT2_SCTRL  0xA10019
#define PORT2_TX     0xA10015
#define PORT2_RX     0xA10017

/* CTRL register — TH pin direction */
#define CTRL_PCS_OUT 0x7F

/* SCTRL flags — baud rate select (bits 7-6) */
#define SCTRL_4800_BPS  0x00
#define SCTRL_2400_BPS  0x40
#define SCTRL_1200_BPS  0x80
#define SCTRL_300_BPS   0xC0

/* SCTRL flags — control */
#define SCTRL_SIN   0x20   /* enable serial input (RX) */
#define SCTRL_SOUT  0x10   /* enable serial output (TX) */
#define SCTRL_RINT  0x08   /* enable receive interrupt (EXT) */

/* SCTRL flags — status (read-only) */
#define SCTRL_RERR  0x04   /* receive error */
#define SCTRL_RRDY  0x02   /* receive data ready */
#define SCTRL_TFUL  0x01   /* transmit buffer full */

/* RX ring buffer size (must be power of 2) */
#define SERIAL_RX_BUF_SIZE 2048

/* Initialise serial port 2 with the given SCTRL flags.
 * Sets TH as output, optionally hooks EXT interrupt. */
void serial_init(u8 sctrl_flags);

/* Read raw SCTRL register value */
u8 serial_sctrl(void);

/* Non-blocking RX check / read (poll mode) */
bool serial_ready_to_receive(void);
u8 serial_receive(void);

/* Non-blocking TX */
void serial_send(u8 data);
bool serial_ready_to_send(void);

/* Blocking TX: spins until TX buffer empty, then sends */
void serial_send_when_ready(u8 data);

/* Callback invoked per drained byte from the RX ring buffer.
 * Called from main context (not ISR) via serial_drain(). */
typedef void (*serial_rx_byte_fn)(u8 byte, void *user);

/* Drain all available bytes from the RX ring buffer.
 * Calls cb(byte, user) for each. Returns count drained. */
int serial_drain(serial_rx_byte_fn cb, void *user);

#endif
