# What depends on the Genesis / SGDK tree?

The repository has **two** largely independent “products” in one tree:

| Layer | Paths | Purpose |
| --- | --- | --- |
| **A — OzWorld (Genesis / SGDK)** | [`clients/genesis/`](../clients/genesis/) — `src/`, `res/`, `inc/`, `out/`, plus [`_compilers/sgdk`](../_compilers/sgdk) | The **68mix / demoscene-style ROM** built with **SGDK**. `clients/genesis/src/main.c` and asm under `src/68mix/` are **not** the TCP BBS. |
| **A′ — Line framing (shared)** | [`clients/common/`](../clients/common/) [`mixnet_line`](../clients/common/mixnet_line.c) | **One** byte-stream line parser + TX helper for bridge/UART work. The Genesis folder **re-exports** it via [`mixnet_line.c`](../clients/genesis/mixnet_line.c) (include of common) so SGDK project layout stays standard. **N64** and **PS1** stubs `#include` the same sources. |
| **B — mixnet (chat hub + clients)** | [`server/`](../server/), [`clients/`](../clients/) (non-SGDK), [`clients/include/`](../clients/include) | **mixnetd** (Rust) and **C clients**: Win9x, POSIX, Amiga (TCP); N64/PS1 **stubs** with self-tests; PS2 stub; **no SGDK** required for the hub or those clients if you are not building the ROM. |

## Do you need to build the Genesis ROM?

- **For mixnetd + only network / stub clients:** **No** — work in `server/`, `clients/` (skip `clients/genesis/src` if you like), `docs/`, and `.cursor` protocol notes.
- **For OzWorld + `rom.bin`:** **Yes** — use **`clients/genesis/`** as the SGDK project root, or from the **repo root** run [`build.bat`](../build.bat) / [`build.ps1`](../build.ps1) (they set cwd to `clients/genesis` for `make`).

A **multi-console “net connector”** in *this* repo is **layer B** (line protocol to **mixnetd** on a PC). **Layer A** is the optional Mega Drive demo; **layer A′** is the portable **`mixnet_line`** stack used by both the MD bridge story and the **N64/PS1** example code.

## “Net only” checkouts

You can keep only: `server/`, `clients/` (drop `genesis` if you do not need the ROM), `docs/`, `.gitignore`, a slim root `README`. The project intentionally keeps the SGDK game under **`clients/genesis/`** and shared line code under **`clients/common/`**.
