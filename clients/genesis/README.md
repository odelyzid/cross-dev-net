# Sega Mega Drive / Genesis — mixnet terminal ROM

This directory is the **SGDK project root**: [`src/`](src/) (C + boot), [`inc/`](inc/), build output in [`out/`](out/) (e.g. `rom.bin` — gitignored). Build from the **repo root** with `build.bat` / `build.ps1` (they `cd` here for `make -f <GDK>\makefile.gen`).

The ROM is a **serial terminal** for mixnet: it talks the **v0 text protocol** over the Mega Drive's on-board serial port (TRS/9-pin or flashcart header) at **4800 bps**, and renders chat in a scrollback UI with an on-screen keyboard.

## Hardware path

1. **Serial port**: MD2 DB9 pins 5 (RX) / 7 (TX) / 8 (GND), or a flashcart UART header — see [`docs/SERIAL-HARDWARE.md`](../../docs/SERIAL-HARDWARE.md).
2. **PC bridge**: Genesis talks **8-bit clean serial**; a PC runs `clients/bridge/mixnet_serial_bridge.py` (COM ↔ TCP) to relay lines to **mixnetd**.

## Files

| File | Role |
| --- | --- |
| [`serial.h`](serial.h) / [`serial.c`](serial.c) | SGDK serial driver: init (4800 bps), RX ISR with ring buffer, `serial_send_when_ready()` TX |
| [`src/serial_terminal.h`](src/serial_terminal.h) / [`src/serial_terminal.c`](src/serial_terminal.c) | Terminal UI: 64-line scrollback, input line, 3-button menu, 10×4 on-screen keyboard |
| [`src/main.c`](src/main.c) | Wiring: serial → `mixnet_line` framing → terminal, TX queue → serial |
| [`mixnet_line.h`](mixnet_line.h) | Re-exports shared [`../common/mixnet_line.h`](../common/mixnet_line.h) (line RX + `mixnet_write_line`) |
| [`mixnet_line.c`](mixnet_line.c) | Includes [`../common/mixnet_line.c`](../common/mixnet_line.c) (single implementation for all clients) |
| [`inc/stddef.h`](inc/stddef.h) | Minimal `stddef.h` for the bare-metal m68k-elf toolchain (SGDK has no libc one) |

## Controls

- **Menu bar (normal mode)** — shown on rows 23–24:
  - `A` — Help, `B` — Nick (`HELLO <nick>`), `C` — Join room (`JOIN <room>`)
  - `LEFT` — Msg (`MSG <text>`), `RIGHT` — `:rooms`, `DOWN` — `:who`, `UP` — scroll back
- **Keyboard mode (`START`)** — 10×4 grid, 2 rows visible per page:
  - `A` — insert char, `B` — backspace, `C` — cancel, `START` — send
  - `UP`/`DOWN` — page/row, `LEFT`/`RIGHT` — column

## Data path (ASCII, same as TCP wire)

- Max line **512** bytes including newline; CR ignored; LF delimits; US-ASCII payload (see spec).
- RX ISR copies bytes into a ring buffer; the main loop drains it through `mixnet_line_rx_byte()`; complete lines go to the terminal scrollback with a `[mm:ss]` timestamp.
- To send: `HELLO nick\n`, `MSG text\n`, etc. (use [`../include/mixnet_proto.h`](../include/mixnet_proto.h)); the terminal queues the line, `main` pumps it byte-by-byte via `serial_send_when_ready()`.

## Bridge (host)

**Windows:** `python clients/bridge/mixnet_serial_bridge.py --serial COM3 --baud 4800` — pipes the console's serial bytes to `mixnetd` on loopback (see [`clients/bridge/README.md`](../bridge/README.md)).

## See also

- [Protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc)
- [Serial pinouts & packet opcodes](../../docs/SERIAL-HARDWARE.md)
- [Server hub](../../server/README.md)
