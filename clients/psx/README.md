# PlayStation 1 (PSX / R3000)

- **Code:** `mixnet_stub.c` **includes** [`../common/mixnet_line.c`](../common/mixnet_line.c) and runs a **self-test** (JOIN write + INFO line parse). Wire **`mixnet_psx_stub_tx`** to your **SIO / parallel / homebrew** byte sink; feed **`mixnet_line_rx_byte`** from the bridge.
- **PSYQ (this machine):** `E:\Emulation\psyq` — set **`PSYQ_ROOT`**, fix `PSPATHS.BAT`. See [`../../docs/TOOLCHAINS.md`](../../docs/TOOLCHAINS.md).
- **Other:** Nugget / **ps1**-style; link flags are project-specific.
- **Spec:** [protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc). Line API: [`../common/README.md`](../common/README.md).

## Cross-build (illustrative)

```text
# One source unit — includes common/mixnet_line.c
mipsel-elf-gcc -c -Iclients/include -Iclients/common clients/psx/mixnet_stub.c
```

(Adjust includes for your project layout; paths must find `../include` and `../common` from the stub.)
