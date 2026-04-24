# N64 (stub)

- **Status:** placeholder `main` only. No link library list is enforced here; use the SDK you’re on (**libultra** / **libdragon** / other).
- **libdragon (this machine):** source tree at `E:\Emulation\libdragon-trunk\libdragon-trunk` — set **`LIBDRAGON_ROOT`**, build the toolchain with upstream `build.sh` (or Docker), then add a target that compiles `mixnet_stub.c` (or a real client) like other examples. See [`../../docs/TOOLCHAINS.md`](../../docs/TOOLCHAINS.md).
- **I/O path:** see [`../genesis/mixnet_line.c`](../genesis/mixnet_line.c) for the line state machine; feed bytes from whatever **bridge** you implement (USB, 64Drive serial, or PC `socat` to TCP).
- **Spec:** [protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc)

## Cross-build (you supply flags)

```text
# Example only — your mips64-elf-ld/flags will differ
mips64-elf-gcc -c clients/n64/mixnet_stub.c -o mixnet_stub.o
```

Link with your `spec`, `entrypoint`, and hardware libs.
