# Build changelog (OzWorld + cross-asm)

Use this as a **lab notebook**: after a successful full build, append an entry (toolchain versions, host OS, one-line result). Protocol and client implementation history lives in **git** and in [`REPO_LAYERS.md`](REPO_LAYERS.md) / [`TOOLCHAINS.md`](TOOLCHAINS.md).

**Index:** [docs/README.md](README.md) lists all `docs/*.md` files.

## Current stack (short)

| Area | Where | Notes |
| --- | --- | --- |
| Hub | `server/` | **mixnetd** — TCP line protocol, default port in `mixnet_config.h`. |
| Line framing | `clients/common/mixnet_line.*` | Used by **Genesis** (re-export), **N64** / **PS1** stubs. |
| Win9x client | `clients/win9x/mixnet.c` | **Winsock**; `-h` / `/?` help. |
| Genesis ROM | `clients/genesis/` | SGDK `out/rom.bin` from root `build.bat` / `build.ps1`. |

## Entries

### Genesis / Mega Drive (SGDK)
- **Status:** pending
- **Command** (from repo root): `build.bat genesis` or `.\build.ps1 -Target Genesis` (runs `make -f <GDK>\makefile.gen release` with cwd `clients/genesis/`)
- **Log:** ✱ Add `stdout`/`stderr` once `clients/genesis/out/rom.bin` is produced.
- **Palette notes:** Keep phasing colors in D0..D3 consistent.

### Amiga 500 / 2000
- **Status:** pending
- **Command:** `vasm … clients/genesis/src/68mix/ozworld.s` (see `build-68mix.sh`)
- **Log:** ✱ Paste assembler/linker output and any ADF / binary hash.

### Sharp X68000
- **Status:** pending
- **Command:** `build-68mix.sh` (X68000 target section)
- **Log:** ✱ Note RAM / resolution target.

### Sega CD / 32X
- **Status:** placeholder
- **Command:** (when wired) same tree as 68mix cross scripts
- **Log:** ✱ Update when the Z80/SH2 or bridge path exists.

---

*Eve — OzWorld / 68mix notebook*
