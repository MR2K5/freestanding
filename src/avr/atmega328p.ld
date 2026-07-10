OUTPUT_FORMAT("elf32-avr")
OUTPUT_ARCH(avr:5) /* ATmega328P belongs to the avr5 core family */

MEMORY
{
  /* 32 KB of Program Flash Memory */
  flash (rx)   : ORIGIN = 0, LENGTH = 32K

  /* 2 KB of Internal SRAM. 
     Note the artificial 0x800000 offset required by the GNU toolchain! 
     0x0000 to 0x00FF are reserved for registers, so usable RAM starts at 0x100 */
  sram  (rw!x) : ORIGIN = 0x800100, LENGTH = 2K
}

SECTIONS
{
  /* 1. Interrupt Vector Table */
  .vectors :
  {
    . = ALIGN(2);
    KEEP(*(.vectors)) /* Keeps vectors from being optimized out */
  } > flash

  /* 2. Program Code and Flash-bound Constants */
  .text :
  {
    . = ALIGN(2);
    *(.text)
    *(.text.*)
    *(.progmem*)   /* AVR-specific: variables forced to stay in Flash */
  } > flash

  .preinit_array :
  {
    . = ALIGN(2);
    PROVIDE(__preinit_array_start = .);
    KEEP(*(SORT(.preinit_array.*)))
    KEEP(*(.preinit_array))
    PROVIDE(__preinit_array_end = .);
  } > flash

  /* 3. Global C++ Constructors (CRITICAL FOR YOUR STDLIB) */
  .init_array :
  {
    . = ALIGN(2);
    PROVIDE(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE(__init_array_end = .);
  } > flash

  /* 4. Initialized Global/Static Variables (The LMA/VMA Sandwich) */
  .data :
  {
    . = ALIGN(2);
    PROVIDE(__data_start = .);
    *(.data)
    *(.data.*)
    *(.rodata)     /* Read-only strings/constants *might* end up here */
    *(.rodata.*)
    PROVIDE(__data_end = .);
  } > sram AT> flash

  /* Expose the exact Flash address where the initial data values are physically stored */
  PROVIDE(__data_load_start = LOADADDR(.data));

  /* 5. Uninitialized Global/Static Variables */
  .bss :
  {
    . = ALIGN(2);
    PROVIDE(__bss_start = .);
    *(.bss)
    *(.bss.*)
    *(COMMON)      /* Fallback for uninitialized loose variables */
    PROVIDE(__bss_end = .);
  } > sram

  /* 6. Define the initial Stack Pointer target (End of SRAM) */
  PROVIDE(__stack_top = ORIGIN(sram) + LENGTH(sram) - 1);
}