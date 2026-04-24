# Documentation

| Doc | What it is |
| --- | --- |
| [REPO_LAYERS.md](REPO_LAYERS.md) | What requires the **Genesis/SGDK** tree vs **mixnet-only** work (`server/`, most of `clients/`). |
| [TOOLCHAINS.md](TOOLCHAINS.md) | **PSYQ** (PS1), **libdragon** (N64), env vars, and where stubs live. |
| [changelog.md](changelog.md) | **Build / release log** (OzWorld, cross-asm) — fill when you complete a real build. |

**Also useful**

- **Protocol (v0):** [../.cursor/.documentation/cross-net/protocol-v0.mdc](../.cursor/.documentation/cross-net/protocol-v0.mdc) — line-oriented ASCII, same wire as **mixnetd**.
- **Shared C:** [`clients/include/`](../clients/include/) (`MIXNET_DEFAULT_PORT`, `MX_*` tokens), [`clients/common/`](../clients/common/) (`mixnet_line` for byte streams on Genesis, N64, PS1).
- **Upstream:** [github.com/odelyzid/cross-dev-net](https://github.com/odelyzid/cross-dev-net) — public mirror of this worktree; root [README.md](../README.md) has build commands.
