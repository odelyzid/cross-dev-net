# AGENTS.md — 68mixCross / cross-dev-net

Compact instruction file for OpenCode agents working in this repo.

## Repository structure

Two largely independent "products" in one monorepo:

| Layer | Paths | Toolchain |
|-------|-------|-----------|
| **mixnetd** (hub) + C clients | `server/`, `clients/` (non-Genesis), `clients/include/`, `clients/common/` | Rust + cargo; Windows needs MSYS2 MinGW on PATH |
| **Genesis ROM** (OzWorld) | `clients/genesis/` (SGDK project), `_compilers/sgdk` | SGDK (`GDK_WIN`), requires `m68k-elf-gcc` |

**Packet framing** (`clients/common/mixnet_packet.*`) is the shared binary packet layer used by all clients (TCP and serial). Legacy line framing (`clients/common/mixnet_line.*`) still exists for v0 text TX on PS1 (Navigator uses `mixnet_write_line()` on its TX path — will be migrated later).

Genesis re-exports `mixnet_line` via thin wrappers at `clients/genesis/mixnet_line.{c,h}` (does not yet use `mixnet_packet` — no UART integration written).

## Key environment variables

| Variable | Purpose |
|----------|---------|
| `GDK_WIN` | SGDK root (default: `_compilers\sgdk`) |
| `MSYS2_ROOT` | MSYS2 install (default: `D:\__SDKs Modding\msys64`) |
| `MIXNETD_IDLE_SEC` | mixnetd idle timeout in seconds |
| `PSYQ_ROOT` / `LIBDRAGON_ROOT` | PS1 / N64 SDK roots (optional, machine-local) |

## Build commands

Always build **from the repo root**, not from subdirectories.

**Windows (cmd):** `build.bat [genesis|server|client|asm|all|clean]` — default: genesis.
**Windows (PowerShell):** `.\build.ps1 -Target [Genesis|Server|Asm68k|Clean|All]` — default: All.

- `build.bat genesis` — SGDK `release` → `clients/genesis/out/rom.bin`
- `build.bat server` — `cargo build --release` → `server/target/x86_64-pc-windows-gnu/release/mixnetd.exe`
- `build.bat client` — builds Win9x and POSIX/MinGW clients via `clients/build_clients.cmd`
- `build.bat all` — Genesis + server
- `build.bat clean` — clears `server/target`, `clients/genesis/out`, `build/genesis`

Per-client build scripts also exist:
- `clients/win9x/build.cmd` — gcc + wsock32
- `clients/posix/build.cmd` — only works on WSL/Linux (POSIX headers)

`make_rom.bat` is a deprecated alias for `build.bat genesis`.

**Unix / Git Bash:** `bash build-68mix.sh` — Genesis + vasm Amiga/X68000 targets.

## Server (mixnetd)

- Single-file Rust at `server/src/main.rs` — **zero external dependencies**.
- Default port: **19677** (or first CLI arg, e.g. `mixnetd 9000`).
- **Auto-detects binary packet mode**: if first received byte is `0x58` → binary packet reader/writer; otherwise → v0 text line mode.
- Run: `server/target/x86_64-pc-windows-gnu/release/mixnetd.exe`
- **Must link with MinGW64** (MSVC `link.exe` not supported). `.cargo/config.toml` hardcodes a path to `x86_64-w64-mingw32-gcc.exe`. Override with `MSYS2_ROOT` or edit the config.
- Tests: `cargo test` inside `server/` — unit tests only (no integration tests).

## C clients

All TCP clients now link `clients/common/mixnet_packet.c` alongside their source. They auto-detect binary packet vs v0 text mode on the first byte from the server.

```text
# Win32 (MinGW):
gcc -std=c99 -O2 -Wall -o clients/win9x/mixnet.exe clients/win9x/mixnet.c clients/common/mixnet_packet.c -lwsock32 -Iclients/include -Iclients/common

# POSIX / Linux / WSL / macOS:
cc -O2 -pthread -o mixnet clients/posix/mixnet.c clients/common/mixnet_packet.c -Iclients/include -Iclients/common

# Amiga (m68k cross):
m68k-amigaos-gcc -O2 -std=c99 -o mixnet clients/amiga/mixnet.c clients/common/mixnet_packet.c -lpthread -lsocket -Iclients/include -Iclients/common
```

Usage: `mixnet <host> <port> [nick] [room]` — `:quit` to send QUIT.

## Bridges (console <-> TCP)

`clients/bridge/mixnet_serial_bridge.py` — COM ↔ mixnetd transparent byte pipe. Requires `pyserial>=3.5` (`pip install -r clients/bridge/requirements.txt`).

Supports physical COM (`--serial COM3 --baud 115200`) or emulator TCP tunnel (`--serial-tcp 127.0.0.1:5678`). Console-side framing handled by `mixnet_line.c` or `mixnet_packet.c` on the console; bridge is framing-agnostic.

## Protocol

Two wire formats, auto-detected on first byte:

- **v0 text** (first byte `!= 0x58`): line-based ASCII, `\n` delimited. Spec: `.cursor/.documentation/cross-net/protocol-v0.mdc`.
- **Binary packet** (first byte `= 0x58`): length-prefixed, 5-byte header (magic+type+flags+u16 length), typed payloads. Spec: `.cursor/.documentation/cross-net/packet-v1.mdc`.

Shared protocol tokens: `clients/include/mixnet_proto.h` (`MX_HELLO`, `MX_JOIN`, `MX_MSG`, etc.).
Config constants: `clients/include/mixnet_config.h` (`MIXNET_DEFAULT_PORT`=19677, `MIXNET_MAX_LINE`=512).
Packet types (C enum): `clients/common/mixnet_packet.h` (`PKT_HELLO`, `PKT_JOIN`, etc.).

## Code conventions

- **C:** 4-space indent (`.editorconfig`), C89-ish (PS1, mixnet_line, mixnet_packet), C99-ish (Win9x, Amiga, POSIX clients), ASCII printable for protocol.
- **Rust:** 4-space indent, edition 2021.
- **No linter/formatter config** beyond `.editorconfig` — no `rustfmt`, `clang-format`, or similar.
- VS Code recommended extensions: `ms-vscode.cpptools`, `zerasul.genesis-code`.
- `.vscode/settings.json` sets default terminal to `Command Prompt` on Windows.

## Git conventions

- **One repo at root** — do not add nested `.git` under `server/`.
- Don't commit: `clients/genesis/out/`, `server/target/`, `build/genesis/`, `clients/genesis/src/ozworld_68mix.s`, `*.exe`, `*.o`, `*.lst`, `*.map`.

## What NOT to do

- Do not add `server/.git` — this repo is one flat tree, no submodules.
- Do not break lines >512 bytes in protocol messages (server returns `ERR line_too_long`).
- Do not assume MSVC linker works for the Rust server — requires MinGW64.
- Do not run build scripts from `server/` cwd without fixing `PATH` — always use root `build.bat`/`build.ps1` or `server/build-mingw.ps1`.
- Do not remove `mixnet_line.c` until all clients (especially PS1 Navigator TX) are migrated to `mixnet_packet` for both RX and TX.

## Toolchain files

- `_compilers/ASM68K/asm68k.exe` — optional, for `ozworld_init_generic_genesis.s`.
- `_compilers/sgdk` — optional bundled SGDK; `build.ps1` also checks `GDK_WIN` and `E:\Emulation\sgdk211`.
- `.cursor/entry-point.mdc` — the "Eve" persona prompt (not canonical project config).
- `.cursor/.documentation/cross-net/` — protocol specs, platform docs, project index.
- `.plans/PACKET-PLAN.md` — implementation plan for the binary packet layer (phases 1-7, with phases 5-6 pending: Genesis UI, Amiga serial).
