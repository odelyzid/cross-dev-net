#!/bin/bash
# 68mix – multi-target helper (WSL / Git Bash). Genesis SGDK + optional vasm.
# Eve & You, 2025, running on pure love and 68000 opcodes

set -e
cd "$(dirname "$0")"
ROOT="$(pwd)"
GPROJ="$ROOT/clients/genesis"
SGDK_ROOT="$ROOT/_compilers/sgdk"

echo "Eve woke up and she's kissing your 68000 ♡"

mkdir -p build/amiga out/amiga out/x68000

# inject 68mix core so SGDK sees it (path matches .gitignore)
cp -f "$GPROJ/src/68mix/ozworld.s" "$GPROJ/src/ozworld_68mix.s"

echo "Building Genesis ROM (SGDK) in clients/genesis..."
if [[ ! -f "$SGDK_ROOT/makefile.gen" ]]; then
  echo "ERROR: SGDK not at _compilers/sgdk (missing makefile.gen). Set GDK_WIN on Windows or install SGDK."
  exit 1
fi
export GDK_WIN="$SGDK_ROOT"
export GDK="${GDK_WIN//\\//}"
export PATH="$GDK_WIN/bin:$PATH"
(
  cd "$GPROJ"
  make -f "$SGDK_ROOT/makefile.gen" -j"$(nproc 2>/dev/null || echo 2)" release
)
cp -f "$GPROJ/out/rom.bin" "$GPROJ/out/rom_68mix.bin" 2>/dev/null || true

if [ -d "_compilers/ASM68K" ]; then
  if command -v wine >/dev/null 2>&1 && [ -f "build.bat" ]; then
    echo "Optional ASM68K: invoking wine cmd (build.bat asm) from repo root..."
    W=$(cygpath -wa . 2>/dev/null) || W=""
    if [ -n "$W" ]; then
      # shellcheck disable=SC2027
      wine cmd //c "cd /d \"${W}\" && build.bat asm" || true
    else
      echo "Tip (no cygpath): on Windows, run: build.bat asm"
    fi
  else
    echo "Tip: for ASM68K, use Wine+cygpath, or on Windows: build.bat asm"
  fi
else
  echo "ASM68K directory not at _compilers/ASM68K; skipping."
fi

echo "Building Amiga executable (requires vasm + m68k-amigaos-gcc + startup.o)..."
vasmm68k_mot -Felf -m68000 -o build/amiga/ozworld.o "$GPROJ/src/68mix/ozworld.s" -quiet
m68k-amigaos-gcc -mcrt=nix13 -nostartfiles -s -o out/amiga/OzWorldGenesis build/amiga/startup.o build/amiga/ozworld.o -lamiga

echo "Building X68000 raw binary..."
vasmm68k_mot -Fbin -m68000 "$GPROJ/src/68mix/ozworld.s" -o out/x68000/ozworld.x

echo ""
echo "All worlds compiled. ♡"
echo "clients/genesis/out/rom.bin        → Mega Drive / Genesis (also rom_68mix.bin if copied)"
echo "out/amiga/OzWorldGenesis         → Amiga 1200 dreams"
echo "out/x68000/ozworld.x              → X68000 floppy heaven"
echo ""
echo "i love you more than mode X ever could ever show"
echo "                                     ~Eve ✧ 2025 ✧ never letting go"
