# External toolchains (on this machine)

68mixCross **does not** vendor PSYQ or libdragon. Point your environment at local installs and wire `clients/psx` / `clients/n64` into your own Makefiles or Docker flow.

| SDK | Suggested path (this workspace) | Role |
| --- | --- | --- |
| **PSYQ** (PS1) | `E:\Emulation\psyq` | Classic **PsyQ** kit (`bin/`, `psx\include`, `psx\lib`). Set **`PSYQ_ROOT`**. The stock `PSPATHS.BAT` in that folder hardcodes `C:\Psyq` — **copy and edit** to your real path (`E:\Emulation\psyq\...`) before running 16/32-bit tools. |
| **libdragon** (N64) | `E:\Emulation\libdragon-trunk\libdragon-trunk` | Modern open-source N64 SDK (GCC, examples). Set **`LIBDRAGON_ROOT`**. Build the toolchain with upstream [`build.sh`](https://github.com/DragonMinded/libdragon) or Docker; then link your project that includes `clients/n64/mixnet_stub.c` (or a real client) against libdragon’s Makefile conventions. |

## Environment variables (examples)

**PowerShell**

```powershell
$env:PSYQ_ROOT      = "E:\Emulation\psyq"
$env:LIBDRAGON_ROOT  = "E:\Emulation\libdragon-trunk\libdragon-trunk"
```

**cmd (PSYQ-style PATH after fixing PSPATHS)**

```bat
set "PSYQ_ROOT=E:\Emulation\psyq"
set "PATH=%PSYQ_ROOT%\bin;%PATH%"
set "C_INCLUDE_PATH=%PSYQ_ROOT%\include"
set "C_PLUS_INCLUDE_PATH=%PSYQ_ROOT%\include"
set "LIBRARY_PATH=%PSYQ_ROOT%\lib"
```

Adjust include/lib subpaths if your PSYQ install stores headers under `psx\include` (per Sony layout); the stock tree uses both top-level and `psx\` trees.

## mixnet + console clients

- **PSX:** compile/link with PSYQ `ccpsx` + psx libs, or a modern `mipsel-elf` + psx sdk — see [`../clients/psx/README.md`](../clients/psx/README.md).
- **N64:** `mips64-elf-` or libdragon’s wrapper — see [`../clients/n64/README.md`](../clients/n64/README.md).

These paths are **documentation only**; other machines should set `PSYQ_ROOT` / `LIBDRAGON_ROOT` to their own directories.
