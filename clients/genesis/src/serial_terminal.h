/* Genesis Terminal UI — scrollback, keyboard entry, menu dispatch.
 * Feeds into mixnet_line TX and receives from serial RX. */
#ifndef SERIAL_TERMINAL_H
#define SERIAL_TERMINAL_H

#include <genesis.h>

#define TERM_SCROLLBACK_LINES 64
#define TERM_LINE_MAX  128
#define TERM_INPUT_MAX 512
#define TERM_VISIBLE_ROWS 22

typedef enum { TERM_MODE_NORMAL, TERM_MODE_KEYBOARD } TermMode;

typedef struct {
	char lines[TERM_SCROLLBACK_LINES][TERM_LINE_MAX];
	u16 head;
	s16 viewport;
} term_scrollback_t;

typedef struct {
	char buf[TERM_INPUT_MAX];
	int pos;
} term_input_t;

/* Initialise terminal UI, clear screen, draw static chrome. */
void terminal_init(void);

/* Feed one received line from the serial framing layer. */
void terminal_push_line(const char *line);

/* Handle joypad event (called from joyEvent). */
void terminal_on_joy(u16 joy, u16 changed, u16 state);

/* Re-render the entire terminal screen. Call once per frame. */
void terminal_render(void);

/* If a line is ready to send, copy it to buf (with \n terminator)
 * and return 1. Returns 0 if nothing pending. */
int terminal_poll_tx(char *buf, int cap);

#endif
