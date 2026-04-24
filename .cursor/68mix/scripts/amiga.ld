MEMORY
{
  CHIPRAM : ORIGIN = 0x000000, LENGTH = 512K
  FASTRAM : ORIGIN = 0x200000, LENGTH = 512K
}

SECTIONS
{
  .text : AT(0x000000)
  {
    *(.text*)
    *(.amiga.copper*)
  } > CHIPRAM

  .data :
  {
    *(.data*)
  } > CHIPRAM
}

