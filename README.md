# 68mixCross

**68mixCross** is a **cross-platform “dev BBS / chat” stack**: a **Rust** TCP hub (**mixnetd**) and **thin C clients** (desktop, Amiga, console) that all speak the same **line-based mixnet v0** protocol. The same design ties a **Sega Mega Drive ROM** and **console bridges** (e.g. N64, **PlayStation 1** text shell) to one server—so you can hack **rooms, nicks, and wire protocol** on the PC, then point hardware or emulators at the same hub.

<p align="center"><img src="cross.png" alt="68mixCross" width="320"/></p>

| Layer | What you get |
| --- | --- |
| **Server** | [`server/`](server/) — **mixnetd** (TCP; default **19677**). The core product for local dev and “LAN party” BBS style sessions. |
| **Genesis (optional)** | [`clients/genesis/`](clients/genesis/) — SGDK **serial terminal ROM**: scrollback chat UI + on-screen keyboard over 4800 bps serial → PC bridge → **mixnetd**. **ROM:** `clients/genesis/out/rom.bin` via root [`build.bat`](build.bat) / [`build.ps1`](build.ps1). |
| **PS1 (PSYQ)** | [`clients/psx/`](clients/psx/) — **Mixnet Navigator**: text UI shell for the same protocol, built for **CCPSX**; see [clients/psx/README.md](clients/psx/README.md) and [clients/psx/BUILD-PS1.md](clients/psx/BUILD-PS1.md). |
| **Other clients** | Win9x, POSIX, Amiga, more stubs in [`clients/`](clients/); shared framing in [`clients/common/`](clients/common/), constants in [`clients/include/`](clients/include/). |

**At a glance**

- You can work on **BBS / chat and mixnetd** without building the **Genesis ROM**; see [`docs/REPO_LAYERS.md`](docs/REPO_LAYERS.md). External SDK setup (PSYQ, **etc.**): [`docs/TOOLCHAINS.md`](docs/TOOLCHAINS.md). AI/editor index: [`.cursor/entry-point.mdc`](.cursor/entry-point.mdc).

## Repository layout

| Path | Purpose |
| --- | --- |
| [`clients/genesis/`](clients/genesis/) | **Sega client**: SGDK project (`src/`, `res/`, `inc/`), `out/rom.bin`, plus [`mixnet_line.c`](clients/genesis/mixnet_line.c) / [`mixnet_line.h`](clients/genesis/mixnet_line.h) |
| [`_compilers/`](_compilers/) | Bundled or linked toolchains; SGDK copy under `_compilers/sgdk` (optional) |
| [`build/`](build/) | Extra build logs / ASM68K outputs (artifacts gitignored) |
| [`server/`](server/) | `mixnetd` — TCP protocol v0 + extensions — [server/README.md](server/README.md) |
| [`clients/`](clients/) | Amiga (full TCP), Win9x (full TCP), POSIX (full TCP), **Genesis** (ROM + line layer), N64 (stub), **PSX** (full serial UI: Mixnet Navigator), PS2 (placeholder); shared headers & framing — [clients/README.md](clients/README.md) |
| [`docs/`](docs/) | [Index](docs/README.md), [changelog](docs/changelog.md), [toolchains](docs/TOOLCHAINS.md), [repo layers](docs/REPO_LAYERS.md) |
| [`.cursor/`](.cursor/) | AI/editor documentation (protocol, platforms, 68k) |

Protocol: [`.cursor/.documentation/cross-net/protocol-v0.mdc`](.cursor/.documentation/cross-net/protocol-v0.mdc)

## Requirements

- **Genesis ROM:** SGDK (GNU `make`, `m68k-elf-gcc` toolchain inside SGDK). Set **`GDK_WIN`** to the SGDK root (folder containing `bin\make.exe` and `makefile.gen`). If you use a copy in-repo, the default is `\_compilers\sgdk`.
- **mixnetd:** [Rust](https://rustup.rs/) (`cargo` on `PATH`) and, on **Windows** with the **GNU** target, a MinGW `bin` directory on `PATH` (e.g. MSYS2 at `D:\__SDKs Modding\msys64`) — see [`server/.cargo/config.toml`](server/.cargo/config.toml) and [`server/build-mingw.ps1`](server/build-mingw.ps1). Override with **`MSYS2_ROOT`**.

## Build (Windows)

From the **repository root** (not `server\` alone):

| Command | Action |
| --- | --- |
| `build.bat` or `build.bat genesis` (alias: [`make_rom.bat`](make_rom.bat)) | SGDK `release` → `clients\genesis\out\` |
| `build.bat server` | `mixnetd` via [`build.ps1`](build.ps1) |
| `build.bat all` | Genesis + server |
| `build.bat clean` | Remove common `clients/genesis/out/`, `build/`, `server/target` artifacts |
| `.\build.ps1` | Same with `-Target` `All` / `Genesis` / `Server` / `Clean` / `Asm68k` |

cmd.exe (not PowerShell) for one-off:

```bat
set "GDK_WIN=E:\path\to\sgdk"
build.bat
```

## Build (PowerShell) examples

```powershell
cd <path-to>\68mixCross
$env:GDK_WIN = "E:\Emulation\sgdk211"   # if not using _compilers\sgdk
$env:MSYS2_ROOT = "D:\__SDKs Modding\msys64"  # if MinGW is not on PATH
.\build.ps1 -Target All
```

## mixnetd run

```text
server\target\x86_64-pc-windows-gnu\release\mixnetd.exe
```

Optional: `MIXNETD_IDLE_SEC=600` to drop idle TCP sessions. Default listen port: **19677** (or pass a port as the first argument).

## Client implementation status

| Client | Path | Type | Status |
|--------|------|------|--------|
| **Win9x** | `clients/win9x/mixnet.c` | Direct TCP (Winsock 1.1) | Full client with threads, all verbs, `:quit` |
| **POSIX** | `clients/posix/mixnet.c` | Direct TCP (BSD sockets) | Full client with pthreads, all verbs |
| **Amiga** | `clients/amiga/mixnet.c` | Direct TCP (AmigaOS sockets) | Full client (same model as POSIX) |
| **PS1** | `clients/psx/` | SIO1 serial ↔ PC bridge | Full UI (Mixnet Navigator: text browser with scrollback, Location bar, menu keys); real SIO1 at 115200 8N1 |
| **Genesis** | `clients/genesis/` | Serial cart/GPIO ↔ PC bridge | Re-exports `mixnet_line` framing; needs UART ISR wiring by developer |
| **N64** | `clients/n64/mixnet_stub.c` | Serial/USB ↔ PC bridge | Stub with self-test; `mixnet_on_server_line` is empty — user must wire UART/PI FIFO |
| **PS2** | `clients/ps2/mixnet_stub.c` | TCP or bridge (TBD) | Placeholder (`int main(){ return 0; }`) — needs PS2SDK + ps2ip/lwIP |
| **Bridge** | `clients/bridge/mixnet_serial_bridge.py` | Serial ↔ TCP relay | Fully functional; two daemon threads, supports physical COM or emulator TCP tunnel |

Build commands (from repo root):

| Client | Command |
|--------|---------|
| **Win32 (MinGW)** | `gcc -std=c99 -O2 -Wall -o clients/win9x/mixnet.exe clients/win9x/mixnet.c -lwsock32` |
| **POSIX** | `cc -O2 -pthread -o mixnet clients/posix/mixnet.c` |
| **Amiga** | `m68k-amigaos-gcc -O2 -std=c99 -o mixnet clients/amiga/mixnet.c -lpthread -lsocket` |

All direct-TCP clients accept: `mixnet <host> <port> [nick] [room]` — `:quit` to send QUIT.

## Serial bridging (console → PC → mixnetd)

Consoles without TCP/IP (Genesis, PS1, N64) connect via serial to a PC running `mixnet_serial_bridge.py`, which relays bytes to and from mixnetd.

```
Console (UART/SIO)  ──serial──▶  bridge.py  ──TCP──▶  mixnetd (port 19677)
     ▲                            (bi-directional        ▲
     └──── serial byte pipe ────── byte relay) ──────────┘
```

- **Bridge is framing-agnostic** — it chunks serial and TCP data in 4096-byte blocks. Line framing (`\n` delimiting) is handled entirely on the console side via [`clients/common/mixnet_line.c`](clients/common/mixnet_line.c).
- **Two serial modes:** `--serial COM3 --baud 115200` (physical) or `--serial-tcp 127.0.0.1:5678` (emulator tunnel for DuckStation/PCSX-Redux).
- **Any RS-232C device** (Amiga, X68000, PC-98) can reuse the same bridge unchanged — just port `mixnet_line.c` with a platform UART driver.
- **Python bridge:** `pip install -r clients/bridge/requirements.txt`; run `python clients/bridge/mixnet_serial_bridge.py --serial COM3 --server 127.0.0.1 --port 19677`.
- **Serial pinouts & wiring:** see [`docs/SERIAL-HARDWARE.md`](docs/SERIAL-HARDWARE.md) for PS1 SIO1, Amiga RS-232, and Genesis flashcart serial pinouts plus the packet opcode catalog.

## Platform coverage / future

| Platform | CPU | Direct TCP | Serial bridge | Notes |
|----------|-----|------------|---------------|-------|
| Windows 9x+ | x86 | ✅ Win9x client | — | Winsock 1.1 |
| Linux / macOS | x86/ARM | ✅ POSIX client | — | BSD sockets + pthreads |
| Amiga (OS 3.x) | 68000 | ✅ Amiga client | ✅ (via RS-232) | AmiTCP/Roadshow |
| PlayStation 1 | R3000 | ❌ | ✅ PS1 SIO1 + Navigator UI | Full text shell |
| Mega Drive/Genesis | 68000 | ❌ | ✅ via `mixnet_line` | Needs UART cart (EverDrive, etc.) |
| Nintendo 64 | R4300 | ❌ | ✅ stub (empty callback) | Needs UART/64Drive/PI FIFO driver |
| PlayStation 2 | EE | ❌ placeholder | ❌ | Needs PS2SDK + ps2ip/lwIP |
| **X68000** | 68000 | ❌ | ✅ (easy — RS-232C built-in) | No client code yet; `build-68mix.sh` already outputs `out/x68000/ozworld.x` demo binary |
| **PC-98** | x86/V30 | ❌ | ✅ (easy — RS-232C built-in) | No client code yet; needs x86 C (unlike rest of repo) |

**Shortest path to add a platform:** serial bridge. Port [`clients/common/mixnet_line.c`](clients/common/mixnet_line.c) (pure C89, CPU-agnostic) with a UART `putbyte`/`getbyte` driver. The Python bridge and mixnetd need zero changes.

## Full cross-asm script (Unix / Git Bash)

[`build-68mix.sh`](build-68mix.sh) can drive extra targets (WSL, vasm, Amiga, X68000); it expects a Unix-like environment and your own toolchains. It also copies `clients/genesis/src/ozworld_68mix.s` (gitignored) during the run.

## Git

**Upstream (this worktree):** [github.com/odelyzid/cross-dev-net](https://github.com/odelyzid/cross-dev-net)

Use the root [`.gitignore`](.gitignore) and [`.gitattributes`](.gitattributes). Do not commit `clients/genesis/out/`, `build/genesis` binaries, `clients/genesis/src/ozworld_68mix.s`, or `server/target/`. **One repo at the project root** — do not add a nested `server/.git`.

To publish from a fresh clone (or a tree without `origin` yet):

```bash
git status   # ensure build artifacts stay untracked
git remote add origin https://github.com/odelyzid/cross-dev-net.git
git branch -M main
git push -u origin main
```

## License

See per-component toolchains (SGDK, etc.). Application source in this tree is offered as-is for porting and learning; add a license file when you pick one for your fork.
