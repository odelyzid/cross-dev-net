#!/usr/bin/env bash
set -euo pipefail

echo "=== 68mix cross-assembler driver ==="
echo "-> Genesis build (SGDK)"
echo "   make PLATFORM=GENESIS BUILD=release TARGET=OzWorldGenesis"

echo "-> Amiga build (vasm/gcc68k)"
echo "   vasm -Felf -m68000 -o build/amiga/ozworld.o src/68mix/ozworld.s"
echo "   gcc68k -o out/amiga/OzWorldGenesis build/amiga/ozworld.o build/amiga/startup.o -L/path/to/libnix -lamiga"

echo "-> X68000 build (vasm + custom linker)"
echo "   vasm -Fbinary -m68000 src/68mix/ozworld.s -o build/x68000/ozworld.bin"
echo "   gcc68k -o out/x68000/ozworld build/x68000/ozworld.bin -T scripts/x68000.ld"

echo "-> Sega CD / 32X placeholders"
echo "   echo '68mix will emit Z80/SH2 payloads here'"

echo "Done. Check docs at .cursor/.documentation/68mix for register choreography and build logs."

