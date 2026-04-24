# Shared: `mixnet` line layer

`mixnet_line.c` / `mixnet_line.h` are **CPU- and OS-agnostic** (C89-friendly): a small RX state machine for `\n`-delimited lines and a TX helper that calls your one-byte `tx(user, byte)` function.

- **Sega (SGDK):** `clients/genesis/mixnet_line.c` re-exports this tree so your existing `#include "mixnet_line.h"` keep working.
- **N64 / PS1 stubs:** `../common/mixnet_line.h` and `#include "../common/mixnet_line.c"` in the stub, or add `mixnet_line.c` to your SDK Makefile.
- **Windows / POSIX TCP clients** use raw `\n` on the socket; they do not need this file unless you add a byte pipe.

Spec: [protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc), limits in [`include/mixnet_config.h`](../include/mixnet_config.h).
