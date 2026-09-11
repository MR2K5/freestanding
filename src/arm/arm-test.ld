/* Minimal Linker Script for Cortex-M4 on QEMU (lm3s6965evb memory layout) */

ENTRY(reset_handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

_stack_size = 0x2000;
_heap_size = 0x4000;

SECTIONS
{
    /* Vector table at the very beginning of Flash (0x00000000) */
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > FLASH

    /* Program code and inlined constants */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)         /* ARM/Thumb interworking glue */
        *(.glue_7t)
        . = ALIGN(4);
    } > FLASH

    /* Read-only data, string literals, and RTTI descriptors */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM EHABI Exception Handling Tables */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    /* ARM EHABI Exception Index Table (Required by the base unwinder) */
    .ARM.exidx :
    {
        . = ALIGN(4);
        __arm_exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __arm_exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    /* C++ Static Constructors table */
    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array*))
        KEEP(*(.ctors*))
        __init_array_end = .;
        . = ALIGN(4);
    } > FLASH

    /* Used by startup.cpp to copy initial values from ROM to RAM */
    __data_load = LOADADDR(.data);

    /* Initialized data placed in RAM, stored in FLASH */
    .data :
    {
        . = ALIGN(4);
        __data_start = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM AT > FLASH

    /* 8-byte Thread Control Block required by ARM Variant I */
    .tcb (NOLOAD) : ALIGN(8)
    {
        __main_tcb = .;
        . += 8;
    } > RAM

    /* TLS Initialized Data Template */
    .tdata : ALIGN(4)
    {
        __tdata_start = .;
        *(.tdata .tdata.* .gnu.linkonce.td.*)
        __tdata_end = .;
    } > RAM AT > FLASH
    __tdata_load = LOADADDR(.tdata);

    /* TLS Uninitialized Data Template */
    .tbss (NOLOAD) : ALIGN(4)
    {
        __tbss_start = .;
        *(.tbss .tbss.* .gnu.linkonce.tb.*)
        *(.tcommon)
        __tbss_end = .;
    } > RAM

    /* Zero-initialized data */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    /* Discard standard C library metadata and debug frames that conflict with EHABI */
    /DISCARD/ :
    {
        *(.ARM.attributes)
        *(.comment)
    }

    .heap (NOLOAD) : ALIGN(8) {
        __heap_start = .;
        . += _heap_size;
        __heap_end = .;
    } > RAM

    /* Stack Section at top of RAM */
    .stack (NOLOAD) : ALIGN(8)
    {
        __stack_bottom = .;
        . += _stack_size;
        . = ALIGN(8);
        __stack_top = .;
    } > RAM
}