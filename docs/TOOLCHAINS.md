# External toolchains (on this machine)

68mixCross **does not** fully vendor PSYQ or the libdragon *toolchain* (only partial SGDK glue under [`_compilers/sgdk`](../_compilers/sgdk)). Set **`PSYQ_ROOT`** / **`LIBDRAGON_ROOT`** to local installs; wire [`clients/psx`](../clients/psx) and [`clients/n64`](../clients/n64) into your own Makefiles, libdragon `Makefile`, or Docker flow.

| SDK | Suggested path (this workspace) | Role |
| --- | --- | --- |
| **PSYQ** (PS1) | `E:\Emulation\psyq` | Classic **PsyQ** (`bin/`, `psx\include`, `psx\lib`). The stock `PSPATHS.BAT` often assumes `C:\Psyq` — **copy and edit** to your real path before running old tools. |
| **libdragon** (N64) | `E:\Emulation\libdragon-trunk\libdragon-trunk` | Open-source N64 SDK. Build the toolchain with upstream [libdragon `build.sh`](https://github.com/DragonMinded/libdragon) or Docker; add [`mixnet_stub.c`](../clients/n64/mixnet_stub.c) to your app target (it **includes** [`clients/common/mixnet_line.c`](../clients/common/mixnet_line.c) in one TU). |
| **SGDK** (Genesis) | your **`GDK_WIN`** or [ `_compilers/sgdk`](../_compilers/sgdk) | Root [`build.bat`](../build.bat) runs `make` with project dir `clients/genesis/`. |
| **Rust + MinGW** (Windows **mixnetd**) | `cargo`, MinGW on `PATH` (often MSYS2) | See [`../server/README.md`](../server/README.md) and [`../server/.cargo/config.toml`](../server/.cargo/config.toml). |

## Environment variables (examples)

**PowerShell**

```powershell
$env:PSYQ_ROOT = "E:\Emulation\psyq"
$env:LIBDRAGON_ROOT = "E:\Emulation\libdragon-trunk\libdragon-trunk"
$env:GDK_WIN = "E:\path\to\sgdk"   # if not using _compilers\sgdk
$env:MSYS2_ROOT = "D:\__SDKs Modding\msys64"  # for GNU Rust link on Windows
```

**cmd (after fixing PSYQ PATH layout)**

```bat
set "PSYQ_ROOT=E:\Emulation\psyq"
set "PATH=%PSYQ_ROOT%\bin;%PATH%"
```

Adjust include/lib subpaths if your PSYQ tree uses `psx\include` vs top-level `include` (varies by rip).

## mixnet + console clients

- **PS1:** [`../clients/psx/README.md`](../clients/psx/README.md) — PSYQ / Nugget-style; stub TX/RX + `mixnet_line` self-test; bridge bytes to a PC running **mixnetd**.
- **N64:** [`../clients/n64/README.md`](../clients/n64/README.md) — same pattern; replace stub TX with your USB/serial/PI path.
- **Win9x:** [`../clients/win9x/mixnet.c`](../clients/win9x/mixnet.c) — real **Winsock 1.1** TCP to **mixnetd** (no `common/mixnet_line` needed for the socket path).
- **Shared constants / verbs:** [`../clients/include/`](../clients/include/) — [`mixnet_config.h`](../clients/include/mixnet_config.h), [`mixnet_proto.h`](../clients/include/mixnet_proto.h).

Paths in this file are **documentation-only**; other machines set roots to their own trees.

**Public repo:** [github.com/odelyzid/cross-dev-net](https://github.com/odelyzid/cross-dev-net)
