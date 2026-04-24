# Amiga 500 / 2000 (68000) TCP client

Target: **AmigaOS 2.x/3.x** on **OCS/ECS/AGA** with a **TCP stack** (AmiTCP, Miami, Roadshow, Genesis, etc.) exposing **BSD-style sockets** (`-lsocket` or bsdsocket.library).

- Source: [`mixnet.c`](mixnet.c) — same session model as [`../posix/mixnet.c`](../posix/mixnet.c) (pthread reader, stdin line sender).
- A500 (chip RAM tight): prefer **small binary**, **no** large stacks; you may need to lower `MIXNET_MAX_LINE` in [`../include/mixnet_config.h`](../include/mixnet_config.h) for extreme constraints.

## Cross-build (example)

**ixemul / pthread + stack socket API** (toolchain-specific):

```bash
m68k-amigaos-gcc -O2 -std=c99 -o mixnet \
  clients/amiga/mixnet.c -lpthread -lsocket
```

**vbcc** (AmiTCP4 SDK) — set include/lib paths to your stack; link **socket** and **pthread** (or single-thread the reader with `WaitSelect` for a future revision).

**Native Amiga (no pthread)** — not in this file; replace `reader_main` with a `struct Task` or `CreateThread`-style path and use `AsyncSocket` or `wait` on the socket. Roadshow and bsdsocket both support `select()`.

## Hardware

Amiga 500/2000: **A590/A2091 SCSI**, **serial** to a PC bridge, or **ISA/PCI bridge** in big-box, or **ParNet**-era tricks — **Ethernet** is the usual path for real TCP. Same client binary once the stack is up.

## See also

- [`m68k-amiga/BUILD.txt`](../m68k-amiga/BUILD.txt) (generic m68k notes)
- [Protocol v0 spec](../../.cursor/.documentation/cross-net/protocol-v0.mdc)
