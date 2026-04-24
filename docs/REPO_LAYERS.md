# What depends on the Genesis / SGDK tree?

The repository has **two** largely independent “products” in one tree:

| Layer | Paths | Purpose |
| --- | --- | --- |
| **A — OzWorld (Genesis / SGDK)** | [`clients/genesis/`](../clients/genesis/) — `src/`, `res/`, `inc/`, `out/`, plus [`_compilers/sgdk`](../_compilers/sgdk) | The **68mix / demoscene-style ROM** built with **SGDK**. `clients/genesis/src/main.c` and asm under `src/68mix/` are **not** the TCP BBS. |
| **B — mixnet (cross-platform chat hub)** | [`server/`](../server/), [`clients/`](../clients/) (other platforms), [`clients/include/`](../clients/include) | **mixnetd** (Rust) plus **C clients** (Win9x, POSIX, Amiga, line layer + stubs for N64/PSX/PS2). **No SGDK** required for the hub or those clients. |

## Do you need to build the Genesis ROM?

- **For mixnetd + only network clients:** **No** — work in `server/`, `clients/` (other than the SGDK subfolder if you are not porting the demo), and docs.
- **For the original Mega Drive project + `rom.bin`:** **Yes** — use **`clients/genesis/`** as the SGDK project root (or point `GDK_WIN` at SGDK and run [`build.bat`](../build.bat) from the **repo root**, which `cd`s into that folder for `make`).

A **multi console “net connector”** in *this* repo is implemented in **layer B**. Layer A is the optional **OzWorld** demo that lives next to the portable **`mixnet_line`** sources in the same **genesis** client directory.

## “Net only” checkouts

You can keep only: `server/`, `clients/` (minus `genesis` if you drop the ROM), `docs/`, `.gitignore`, and a slim root `README`. The current project keeps the SGDK game tree under **`clients/genesis/`** on purpose.
