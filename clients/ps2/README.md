# PlayStation 2 (stub, EE)

- **Status:** `mixnet_stub.c` — placeholder. Real work needs **PS2SDK** (or your vendor tree), optional **ps2ip** / **lwIP** for TCP, or a **bridge** to mixnetd (same as N64/PSX/Genesis).
- **IOP vs EE:** networking often lands on the IOP; this stub is **EE** `main` for simplicity. Split later if you service sockets on the IOP.

## Build (illustrative)

```text
ee-gcc -c clients/ps2/mixnet_stub.c
```

(Your Makefile will add `crt0`, linker script, and `-l` set.)

## Spec

[protocol v0](../../.cursor/.documentation/cross-net/protocol-v0.mdc)
