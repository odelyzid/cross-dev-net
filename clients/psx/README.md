# PlayStation 1 (PSX) — Mixnet Navigator (“Netscape” hub)

A **text-mode “browser” shell** for **mixnetd**: title bar, **Location** bar (`mixnet://host:port/room` — logical address for your PC bridge; the PS1 does not run raw TCP in this source tree), **menu** keys [1]–[6], services (**ROOMS**, **WHO**, **PART**), and a **scroll** area for `INFO` / `PRIVMSG` lines — same line protocol as [`../../server/`](../../server) (default port in [`include/mixnet_config.h`](../include/mixnet_config.h)).

## Source files

| File | Role |
| --- | --- |
| [`mixnet_navigator.c`](mixnet_navigator.c) | Hub / UI state, URL parser, `mixnet_nav_*` API |
| [`mixnet_navigator.h`](mixnet_navigator.h) | Public include |
| [`mixnet_stub.c`](mixnet_stub.c) | `main`, byte TX buffer, [optional] `mixnet_psx_*` display stubs |
| [`mixnet_psx.h`](mixnet_psx.h) | Hooks for your **FntPrint** / GPU text layer |
| [`../common/mixnet_line.c`](../common/mixnet_line.c) | Line framing over the **bridge** (included from `main` T.U.) |

**Build** (illustrative — add both `.c` to your project):

```text
ccpsx -c -I. mixnet_navigator.c
ccpsx -c -I. mixnet_stub.c
# link with lib, crt, etc.
```

On a **PC host** (sanity only): `gcc -std=c99 -c mixnet_*.c` and link — `main` returns `0` if line self-test + in-memory **navigator demo** pass.

## User commands (in-app)

| Input | Action |
| --- | --- |
| `:h` | Help (scroll) |
| `:q` / `:quit` | Send **QUIT** to server; exit when you wire the loop to `mixnet_nav_want_quit()` |
| `:loc mixnet://host:port/room` | Set **location** (display + JOIN target) |
| `:nick name` | Default nick for **HELLO** |
| `:g` / `:go` | Send `HELLO <nick>` and `JOIN <room>` if set |
| `:rooms` / `:who` / `:part` / `:ping` | Same as wire verbs |
| `:j room` / `:m text` | **JOIN** / **MSG** (needs hello + in-room for `:m`) |
| `1` … `6` | Hub shortcuts (menu in scroll + help) |
| *Raw* | `HELLO n`, `JOIN r`, `MSG t`, `PING`, `QUIT` … passed through |

**Bridge:** a PC (or RPi) process reads the TX buffer / serial from the console and opens **TCP to mixnetd**; the reverse path feeds `mixnet_line_rx_byte` until a line completes, then `mixnet_nav_on_incoming_line`.

## Tooling

- **PSYQ:** e.g. `E:\Emulation\psyq` — set **`PSYQ_ROOT`**, fix `PSPATHS.BAT` — [TOOLCHAINS](../../docs/TOOLCHAINS.md)
- **Protocol:** [protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc)
