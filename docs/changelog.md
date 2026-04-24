# OzWorldGenesis Build Changelog

Record every successful 68mix target run here. Include timestamp, toolchain versions, and any palette/copper notes. Append a new entry immediately after each build completes.

## Entries

### Genesis / Mega Drive (SGDK)
- **Status**: pending
- **Command** (from repo root): `build.bat genesis` or `.\build.ps1 -Target Genesis` (runs `make -f <GDK>\makefile.gen release` with cwd `clients/genesis/`)
- **Log**: ✱ Fill with `stdout`/`stderr` once `clients/genesis/out/rom.bin` is produced.
- **Palette notes**: Keep phasing colors in D0..D3 consistent.

### Amiga 500 / 2000
- **Status**: pending
- **Command**: `vasm -Felf -m68000 -o build/amiga/ozworld.o clients/genesis/src/68mix/ozworld.s && …` (see `build-68mix.sh`)
- **Log**: ✱ Paste the assembler/linker output and the resulting ADF info.
- **Copper notes**: Annotate each Denise write per the 55ns stripe.

### Sharp X68000
- **Status**: pending
- **Command**: `vasm -Fbinary -m68000 clients/genesis/src/68mix/ozworld.s -o build/x68000/ozworld.bin && …` (see `build-68mix.sh`)
- **Log**: ✱ Drop the `gcc68k` pass and `xmount` copy info here.
- **VRAM notes**: Mention the 512×240 vs 640×400 target.

### Sega CD / 32X
- **Status**: placeholder
- **Command**: `scripts/build-68mix.sh` (placeholder for Z80/SH2 pipeline)
- **Log**: ✱ Update when the shared RAM handoff is wired.

// Eve * 2025 * forever yours in 640x480

