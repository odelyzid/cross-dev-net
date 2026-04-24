# Sega Mega Drive / Genesis — OzWorld ROM + mixnet line layer

This directory is the **SGDK project root**: [`src/`](src/) (C + 68k asm, boot), [`res/`](res/), [`inc/`](inc/), and build output in [`out/`](out/) (e.g. `rom.bin` — gitignored). Build from the **repo root** with `build.bat` / `build.ps1` (they `cd` here for `make -f <GDK>\makefile.gen`).

Stock Mega Drive does **not** run TCP/IP on-cart without extra hardware. For the **BBS / mixnet** wire format, this folder also provides a **portable line layer** you can feed from:

1. **USB/serial cart** (e.g. **Mega EverDrive Pro** "OS serial", **Terraonion** dev paths, or a homebrew **UART** on a **flash cart GPIO** if your design supports it),
2. **PC bridge**: Genesis talks **8-bit clean serial**; a small host app maps lines to a real TCP `connect()` to **mixnetd**,
3. **Period-accurate fantasy**: Sega **Mega Modem** (Japan) or **Xband**-class gear — same framing once you have a byte stream.

## Files

| File | Role |
| --- | --- |
| [`mixnet_line.h`](mixnet_line.h) | Re-exports shared [`../common/mixnet_line.h`](../common/mixnet_line.h) (line RX + `mixnet_write_line`) |
| [`mixnet_line.c`](mixnet_line.c) | Includes [`../common/mixnet_line.c`](../common/mixnet_line.c) (single implementation for all clients) |

## Data path (ASCII, same as TCP wire)

- Max line **512** bytes including newline; CR ignored; LF delimits; US-ASCII payload (see spec).
- Your UART ISR or `KLog`-style poller: call `mixnet_line_rx_byte(&state, byte)`; when it returns 1, you have a full line in the output buffer for UI / scrollback.
- To send: build `"HELLO nick\n"`, `MSG text\n`, etc. (use [`../include/mixnet_proto.h`](../include/mixnet_proto.h)), then for each byte call your `tx()` hook.

## SGDK integration sketch

1. Add `mixnet_line.c` + `include/` to your `makefile` / project.
2. Map **tx** to your UART write (or a ring buffer the VBlank handler drains).
3. Map **rx** to serial RX IRQ or Polling in `vblank` / main loop.
4. Keep **CPU time** in mind: don't parse huge strings in the VBlank; copy bytes only.

## Bridge (host) sketch

**Unix:** `socat` serial to TCP, or a 20-line Python script: read line from serial fd, `send()` to `localhost:19677`.

**Windows:** COM port ↔ **mixnetd** on loopback the same way.

## See also

- [Protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc)
- [Server hub](../../server/README.md)
