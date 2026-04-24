# N64 (line layer + self-test)

- **Code:** `mixnet_stub.c` includes [`../common/mixnet_line.c`](../common/mixnet_line.c) in one translation unit, runs a small **TX/RX self-test** at startup (returns non-zero on failure). Replace `mixnet_n64_stub_tx` with your **UART / USB / PI FIFO** byte writer; in your main loop, call `mixnet_line_rx_byte` for each **RX** byte and `mixnet_on_server_line` when a line completes.
- **libdragon (this machine):** tree at `E:\Emulation\libdragon-trunk\libdragon-trunk` — set **`LIBDRAGON_ROOT`**, build the toolchain, add a build rule for `mixnet_stub.c` like other examples. See [`../../docs/TOOLCHAINS.md`](../../docs/TOOLCHAINS.md).
- **Spec:** [protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc). Shared line API: [`../common/README.md`](../common/README.md).

## Cross-build (you supply flags)

```text
# One TU — common/mixnet_line.c is #included from mixnet_stub.c
mips64-elf-gcc -c -I. -I.. clients/n64/mixnet_stub.c -o mixnet_stub.o
```

…then link with your `spec` / `entrypoint` and SDK libs. Paths `-I` should reach `clients/include` and `clients/common` (adjust `-I` to your project root).
