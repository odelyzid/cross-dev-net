# AGENTS.md — 68mixCross / cross-dev-net

Compact instruction file for OpenCode agents working in this repo.

## Repository structure

Two largely independent "products" in one monorepo:

| Layer | Paths | Toolchain |
|-------|-------|-----------|
| **mixnetd** (hub) + C clients | `server/`, `clients/` (non-Genesis), `clients/include/`, `clients/common/` | Rust + cargo; Windows needs MSYS2 MinGW on PATH |
| **Genesis ROM** (OzWorld) | `clients/genesis/` (SGDK project), `_compilers/sgdk` | SGDK (`GDK_WIN`), requires `m68k-elf-gcc` |

**Line framing** (`clients/common/mixnet_line.*`) is shared by Genesis, N64, PS1 stubs. Genesis re-exports it via thin wrappers at `clients/genesis/mixnet_line.{c,h}`.

## Key environment variables

| Variable | Purpose |
|----------|---------|
| `GDK_WIN` | SGDK root (default: `_compilers\sgdk`) |
| `MSYS2_ROOT` | MSYS2 install (default: `D:\__SDKs Modding\msys64`) |
| `MIXNETD_IDLE_SEC` | mixnetd idle timeout in seconds |
| `PSYQ_ROOT` / `LIBDRAGON_ROOT` | PS1 / N64 SDK roots (optional, machine-local) |

## Build commands

Always build **from the repo root**, not from subdirectories.

**Windows (cmd):** `build.bat [genesis|server|asm|all|clean]` — default: genesis.
**Windows (PowerShell):** `.\build.ps1 -Target [Genesis|Server|Asm68k|Clean|All]` — default: All.

- `build.bat genesis` — SGDK `release` → `clients/genesis/out/rom.bin`
- `build.bat server` — `cargo build --release` → `server/target/x86_64-pc-windows-gnu/release/mixnetd.exe`
- `build.bat all` — both above
- `build.bat clean` — clears `server/target`, `clients/genesis/out`, `build/genesis`

`make_rom.bat` is a deprecated alias for `build.bat genesis`.

**Unix / Git Bash:** `bash build-68mix.sh` — Genesis + vasm Amiga/X68000 targets.

## Server (mixnetd)

- Single-file Rust at `server/src/main.rs` — **zero external dependencies**.
- Default port: **19677** (or first CLI arg, e.g. `mixnetd 9000`).
- Protocol: line-based ASCII (v0 + mixnetd extensions: `PART`, `WHO`, `ROOMS`, idle timeout).
- Run: `server/target/x86_64-pc-windows-gnu/release/mixnetd.exe`
- **Must link with MinGW64** (MSVC `link.exe` not supported). `.cargo/config.toml` hardcodes a path to `x86_64-w64-mingw32-gcc.exe`. Override with `MSYS2_ROOT` or edit the config.
- Tests: `cargo test` inside `server/` — unit tests only (no integration tests).

## C clients (no server build needed)

```text
# Win32 (MinGW):
gcc -std=c99 -O2 -Wall -o clients/win9x/mixnet.exe clients/win9x/mixnet.c -lwsock32

# POSIX / Linux / WSL / macOS:
cc -O2 -pthread -o mixnet clients/posix/mixnet.c
```

Usage: `mixnet <host> <port> [nick] [room]` — `:quit` to send QUIT.

## Bridges (console <-> TCP)

`clients/bridge/mixnet_serial_bridge.py` — COM ↔ mixnetd. Requires `pyserial>=3.5` (`pip install -r clients/bridge/requirements.txt`).

## Protocol

Canonical spec: `.cursor/.documentation/cross-net/protocol-v0.mdc`.
Shared tokens: `clients/include/mixnet_proto.h` (`MX_HELLO`, `MX_JOIN`, `MX_MSG`, etc.).
Config constants: `clients/include/mixnet_config.h` (`MIXNET_DEFAULT_PORT`=19677, `MIXNET_MAX_LINE`=512).

## Code conventions

- **C:** 4-space indent (`.editorconfig`), C99-ish (Win9x client is ANSI C), ASCII printable for protocol.
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

## Toolchain files

- `_compilers/ASM68K/asm68k.exe` — optional, for `ozworld_init_generic_genesis.s`.
- `_compilers/sgdk` — optional bundled SGDK; `build.ps1` also checks `GDK_WIN` and `E:\Emulation\sgdk211`.
- `.cursor/entry-point.mdc` — the "Eve" persona prompt (not canonical project config).
