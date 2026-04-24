.include "MACROS.S"

.section .text
.global OzWorldGenesis_entry
.type OzWorldGenesis_entry,@function
OzWorldGenesis_entry:
    ; entrypoint for 68mix outputs, shares the demo state machine for every platform
    jsr renderDemoState
    rts

.section .data
.align 4
demo_state_table:
    .long demo_vector_phase
    .long demo_copper_phase
    .long demo_checker_phase

demo_vector_phase:
    ; sample macro usage for vector stage
    VECTOR_STEP 0x2000,FIX16(60)
    .word 0x0000 ; placeholder for vector count

demo_copper_phase:
    ; copper writes described per-section
    COPPER_WRITE 0x00, 0x00
    .word 0x0000

demo_checker_phase:
    ; checkerboard uses framebuffer pointer
    X68000_FRAMEBUFFER checker_coords
checker_coords:
    .word 0x0000, 0x0000

.section .genesis.banks
GENESIS_ROM_BANK 0,genesis_start
genesis_start:
    jsr demo_state_table
    rts

.section .amiga.copper
AMIGA_COPPER_LIST amiga_copper
amiga_copper:
    ; copper list placeholder; add MOVE instructions targeting Denise registers
    .long 0x00000000


