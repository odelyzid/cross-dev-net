# PlayStation 1 (PSX / R3000 stub)

- **Status:** `mixnet_stub.c` — `main` placeholder only. Wire real networking only if you integrate **sockets** (homebrew) or a **byte bridge**; retail PS1 has no public TCP to the world.
- **Line layer:** use [`../genesis/mixnet_line.c`](../genesis/mixnet_line.c) (CPU-agnostic) if you have a one-byte read/write path.
- **PSYQ (this machine):** a tree lives at `E:\Emulation\psyq` — set **`PSYQ_ROOT`** to that path and fix `PSPATHS.BAT` (stock file assumes `C:\Psyq`). See [`../../docs/TOOLCHAINS.md`](../../docs/TOOLCHAINS.md).
- **Other:** modern **psx** / **Nugget**-style; link flags are project-specific.

## Cross-build (illustrative)

```text
# Replace with your mipsel-elf / ps1 toolchain
cc -c clients/psx/mixnet_stub.c
```

## Spec

[protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc)
