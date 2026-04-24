# 68mixCross

**68mixCross** is a **cross-platform “dev BBS / chat” stack**: a **Rust** TCP hub (**mixnetd**) and **thin C clients** (desktop, Amiga, console) that all speak the same **line-based mixnet v0** protocol. The same design ties a **Sega Mega Drive ROM** and **console bridges** (e.g. N64, **PlayStation 1** text shell) to one server—so you can hack **rooms, nicks, and wire protocol** on the PC, then point hardware or emulators at the same hub.

| Layer | What you get |
| --- | --- |
| **Server** | [`server/`](server/) — **mixnetd** (TCP; default **19677**). The core product for local dev and “LAN party” BBS style sessions. |
| **Genesis (optional)** | [`clients/genesis/`](clients/genesis/) — SGDK + [`mixnet_line`](clients/genesis/mixnet_line.c) for serial/bridge; **ROM:** `clients/genesis/out/rom.bin` via root [`build.bat`](build.bat) / [`build.ps1`](build.ps1). |
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
| [`clients/`](clients/) | Amiga, Win9x, POSIX, **Genesis** (ROM + line layer), N64, **PSX** (Mixnet Navigator + [`BUILD-PS1.md`](clients/psx/BUILD-PS1.md)), PS2 stubs; shared headers in [`include/`](clients/include/) — [clients/README.md](clients/README.md) |
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

## Clients (dev)

- **Win32 / Win9x style:** build [`clients/win9x/mixnet.c`](clients/win9x/mixnet.c) with `cl` + `wsock32` or `gcc` + `-lwsock32`.
- **Linux / WSL / macOS:** `cc -O2 -pthread -o mixnet clients/posix/mixnet.c`

## Full cross-asm script (Unix / Git Bash)

[`build-68mix.sh`](build-68mix.sh) can drive extra targets (WSL, vasm, Amiga, etc.); it expects a Unix-like environment and your own toolchains. It also copies `clients/genesis/src/ozworld_68mix.s` (gitignored) during the run.

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
