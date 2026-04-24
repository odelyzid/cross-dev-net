# Project index (68mixCross workspace)

One-line registry so the tour menu and replication rules stay linked to real trees.

| Program / tree | Path | Rule |
| --- | --- | --- |
| OzWorldGenesis (SGDK + 68mix) | [`clients/genesis/`](../../clients/genesis) (`src/`, `res/`, `inc/`), root `build.bat` / `build.ps1`, [`README.md`](../../README.md) | Default demo; follow `.documentation/platforms/index.mdc`. Set `GDK_WIN` to SGDK if not using `_compilers\sgdk`. `make` runs with cwd `clients\genesis`. |
| Cross-net hub (docs) | `.cursor/.documentation/cross-net/` | Protocol v0, roadmap, Win9x notes. |
| Hub server (mixnetd) | `server/` | From `server/`: `.\build-mingw.ps1` or set MinGW `bin` on PATH, then `cargo build --release`. Binary: `target\x86_64-pc-windows-gnu\release\mixnetd.exe`. Env: `MIXNETD_IDLE_SEC`. Implements v0 + mixnetd extensions in `protocol-v0.mdc`. |
| Win9x / Win32 client | `clients/win9x/mixnet.c` | Winsock 1.1 console; `cl ... /link wsock32.lib` or MinGW `-lwsock32`. Args: `host port [nick room]`. |
| POSIX / dev client | `clients/posix/mixnet.c` | `cc -pthread -o mixnet mixnet.c`; same args. |
| Amiga 500/2000 + stubs | `clients/amiga/`, `clients/m68k-amiga/`, `clients/n64/`, `clients/psx/`, `clients/ps2/`, `clients/genesis/` | [clients README](../../clients/README.md). |

## Small rules
- When you add a new top-level program folder, append a row here and mention it in `.documentation/menu.mdc` if the tour should surface it.
- Toolchain pins for each tree live in `docs/changelog.md` and [`docs/TOOLCHAINS.md`](../../docs/TOOLCHAINS.md) (PSYQ, libdragon). [`docs/REPO_LAYERS.md`](../../docs/REPO_LAYERS.md) explains when the **Genesis/SGDK** tree is required vs mixnet-only work.
