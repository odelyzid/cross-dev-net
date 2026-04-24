ENTRY(OzWorldGenesis_entry)
SECTIONS
{
  . = 0x000000;
  .text :
  {
    *(.text*)
    *(.genesis.bank*)
  } > ROM
}

