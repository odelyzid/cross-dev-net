# 68mixCross clients

Console / embedded clients for the same **line protocol** as [`../server`](../server) (default port **19677**; see [`include/mixnet_config.h`](include/mixnet_config.h) and [`include/mixnet_proto.h`](include/mixnet_proto.h)).

| Directory | Target | Notes |
| --- | --- | --- |
| [`win9x/`](win9x/) | Windows 95+ / Win32 | Winsock 1.1, `_beginthreadex` + stdin. |
| [`posix/`](posix/) | Linux, macOS, WSL, Amiga+ixemul | `pthread`, BSD `socket`. |
| [`amiga/`](amiga/) | **Amiga 500 / 2000** (68000) + AmigaOS 3.x TCP | Same model as POSIX; see [amiga/README.md](amiga/README.md). |
| [`genesis/`](genesis/) | **Sega Mega Drive** — **ROM (OzWorld) + serial bridge** | SGDK `src/`, `res/`, `inc/`, `out/rom.bin` and `mixnet_line` for UART/TCP bridges; [genesis/README.md](genesis/README.md). |
| [`n64/`](n64/) | Nintendo 64 | Stub + README; bridge / USB TBD. |
| [`psx/`](psx/) | PlayStation 1 (R3000) | Stub + README; bridge or Yaroze-style TBD. |
| [`ps2/`](ps2/) | PlayStation 2 (EE) | Stub + README; ps2ip / bridge TBD. |
| [`m68k-amiga/`](m68k-amiga/) | Pointers | [BUILD.txt](m68k-amiga/BUILD.txt) — points at `amiga/`. |

## Usage (text clients)

```text
mixnet <host> <port> [nick] [room]
```

If `nick` and `room` are set, the client sends `HELLO` then `JOIN` before your lines. Type **`:quit** to send `QUIT`.

## Build (quick)

**Win32 (MinGW, from repo root):** `gcc -O2 -o clients/win9x/mixnet.exe clients/win9x/mixnet.c -lwsock32`

**POSIX:** `cc -O2 -pthread -o mixnet clients/posix/mixnet.c`

**Amiga (m68k):** `m68k-amigaos-gcc` + `-lpthread -lsocket` — see [amiga/README.md](amiga/README.md).

**Genesis:** compile `genesis/mixnet_line.c` with your SGDK project; see [genesis/README.md](genesis/README.md).

**N64 / PSX / PS2:** there is no shared Makefile in this repo; link stubs into your SDK project.

## Spec and server

- Protocol: [../.cursor/.documentation/cross-net/protocol-v0.mdc](../.cursor/.documentation/cross-net/protocol-v0.mdc)
- Hub: [../server/README.md](../server/README.md)

## Git

Do not commit `*.exe` or local `mixnet` binaries — [root `.gitignore`](../.gitignore).
