ENTRY(OzWorldGenesis_entry)
SECTIONS
{
  . = 0x000000;
  .text :
  {
    *(.text*)
  } > ROM

  .framebuffer :
  {
    *(.x68000.framebuffer*)
  } > ROM
}

