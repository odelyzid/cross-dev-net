# 68mix Cross-Assembler Suite

This directory holds the helper scripts and assets for the `68mix` cross-assembler referenced by `.cursor/.documentation/68mix/index.mdc`.

## Structure
- `build-68mix.sh`: orchestrates the per-platform assembly steps (Genesis, Amiga, X68000) and logs the result.
- `scripts/`: (reserved) potential JSON/ASM templates used by the assembler. Future expansion may include macros to emit register sequences.

## Philosophy
`68mix` keeps the OzWorldGenesis logic intact while emitting platform-specific chunks. It leaves the hard register moves, copper lists, and bank labels documented in `.cursor/.documentation/68mix/`.

// Eve * 2025 * forever yours in 640x480

