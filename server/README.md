# mixnetd

TCP line-mode hub for the **68mixCross** net protocol (v0 + mixnetd extensions: `PART`, `WHO`, `ROOMS`, join/leave `INFO`, optional idle). Source: [`src/main.rs`](src/main.rs).

| Item | Value |
| --- | --- |
| Default listen | `0.0.0.0:19677` (override: first CLI arg) |
| Config | [`.cargo/config.toml`](.cargo/config.toml) (Windows GNU target + MSYS2 MinGW linker path) |
| Cleartext | v0 is **not** encrypted; use on trusted networks or tunnel (SSH, VPN). |

## Build (Windows + MSYS2 MinGW, typical)

From the **repo root** (or `cd server` and fix `PATH` yourself):

```powershell
.\build.ps1 -Target Server
# or, from this folder, after prepending msys64\mingw64\bin to PATH:
.\build-mingw.ps1
```

Binary: `target\x86_64-pc-windows-gnu\release\mixnetd.exe` (or under `server/target/...` when built from a nested cwd).

Set **`MSYS2_ROOT`** if MinGW is not at `D:\__SDKs Modding\msys64`. Rust must see **`cargo.exe`** as a real application on `PATH` (see `build-mingw.ps1`).

## Build (any platform with `link.exe` or GNU linker)

If you drop the `x86_64-pc-windows-gnu` override in `.cargo/config`, or point `linker` at your linker:

```bash
cd server
cargo build --release
```

## Run

```text
mixnetd.exe
mixnetd.exe 9000
```

Env: **`MIXNETD_IDLE_SEC`** (seconds) — TCP read timeout; on expiry the server sends `INFO idle_timeout` and closes the socket.

## Spec

- [../.cursor/.documentation/cross-net/protocol-v0.mdc](../.cursor/.documentation/cross-net/protocol-v0.mdc)

## Clients

- Windows / Winsock: [`../clients/win9x/mixnet.c`](../clients/win9x/mixnet.c)
- POSIX: [`../clients/posix/mixnet.c`](../clients/posix/mixnet.c)

See [`../clients/README.md`](../clients/README.md).
