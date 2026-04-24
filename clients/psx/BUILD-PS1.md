# Build a testable PS1 image (PSYQ on Windows)

The official PSYQ Win32 tools (**SDevTC / CCPSX 3.06**) look for configuration as **`PSYQ.INI` or `sn.ini` in the `bin` folder** next to `CCPSX.EXE` (see [Retro: PSYQ SDK setup](https://www.retroreversing.com/psyq-sdk-setup)). They install cleanly when the tree is on **`C:\Psyq`** (library at `C:\Psyq\lib`, includes at `C:\Psyq\include` — not only `psx\lib`).

## 1) Put PSYQ where CCPSX expects it

If your kit lives on another drive (e.g. `E:\Emulation\psyq`), create a **directory junction** (Admin **cmd** once):

```bat
mklink /J C:\Psyq E:\Emulation\psyq
```

Or install/copy the PSYQ tree to **`C:\Psyq`** so you have at least:

- `C:\Psyq\bin\CCPSX.EXE`, `PSYLINK.EXE`, `CPE2X.EXE`, `CPPPSX.EXE`, `CC1PSX.EXE`, `ASPSX.EXE`
- `C:\Psyq\lib\` (e.g. `LIBC.LIB`, `LIBSN.LIB`, `2MBYTE.OBJ` may live in `C:\Psyq\psx\lib` on your rip; adjust the linker script in `build-psyq.bat` if your `2mbyte` path differs)
- `C:\Psyq\include\` (e.g. `STRING.H`, `STDLIB.H`)

Copy [`PSYQ.INI`](PSYQ.INI) to **`C:\Psyq\bin\PSYQ.INI`** and edit the paths if your layout differs (some trees use `psx\lib` / `psx\include` only — set those paths consistently in the INI and in the link script).

## 2) Run the build

From a normal **Command Prompt** (the script uses 32-bit `cmd` when possible):

```bat
set PSYQ=C:\Psyq
cd /d E:\Emulation\SGDK_NEW\project\68mixCross\clients\psx
build-psyq.bat
```

Outputs (default):

- `out\mixnet.cpe`
- `out\mixnet.exe` — PS-X executable, if `CPE2X` succeeded

## 3) Test in an emulator

- **DuckStation / PCSX-Redux** / **no$psx**: use **Run executable** (or an ISO you build) and point at **`out\mixnet.exe`**, or the **`.CPE`** if the emu accepts it.
- The ROM **runs the line-layer self-test and navigator demo** in `main` then **exits** — so you can confirm it boots; wire SIO/pad to your real `main` loop later.

## If CCPSX still says: `can't read 'sn.ini' or 'psyq.ini'`

SDevTC **3.06** on Win32: **`sn.ini` / `psyq.ini` in the PSYQ `bin` folder is not enough by itself. You also need the environment variables **`SN_PATH`**, **`PSYQ_PATH`**, and (per `bin\DOC\CCPSX.TXT`) **`COMPILER_PATH`**, etc., pointing at that `bin\` (and library/include paths for the `stdlib=` line in the INI). The batch sets these before every `CCPSX` run.

- Ensure a valid `sn.ini` and `psyq.ini` in **`%SN_PATH%`** (usually `C:\Psyq\bin` when a junction is used), **ASCII, CRLF**, consistent paths (often **`C:\Psyq\...`**, not `E:\` — 32-bit tools can reject non–`C:` INI).
- The link step uses **`ccpsx -Xo$80010000 … -omixnet.cpe,sym`** (not a `PSYLINK` `.lnk` file), same idea as `psx\sample\serial\SIO\TUTO2\MAKEFILE.MAK`.
- **`CPE2X.EXE`** in many rips is **16-bit** and will not run on 64-bit Windows. **`out\mixnet.cpe`** is still a valid, testable build for emulators; run **`CPE2X`** in a 16/32-bit or Win9x VM to get **`mixnet.exe`**.

## Cross-env without PSYQ

A modern `mipsel-elf` + ps1 `crt0` / LDFLAGS is **not** wired in this repo; use a community PS1 SDK (e.g. Nugget) and port the two `.c` files + `common/mixnet_line` into that project if you do not have PSYQ.
