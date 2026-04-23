/* Tell the linker where Flash and RAM are on the STM32F401RE */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

SECTIONS
{
    /* .text = your code, goes into Flash */
    .text :
    {
        KEEP(*(.isr_vector))   /* Vector table MUST be first in Flash */
        *(.text*)              /* All code from all files */
        *(.rodata*)            /* Read-only data (string literals, const) */
    } > FLASH

    /* .data = initialized global variables
       Stored in Flash at load time, copied to RAM at startup */
    .data :
    {
        _sdata = .;            /* Symbol: start of data in RAM */
        *(.data*)
        _edata = .;            /* Symbol: end of data in RAM */
    } > RAM AT> FLASH          /* Lives in RAM, but loaded from Flash */

    _la_data = LOADADDR(.data); /* Where .data lives in Flash (load address) */

    /* .bss = uninitialized globals, zeroed at startup */
    .bss :
    {
        __bss_start__ = .;     /* Symbol startup code uses to find BSS start */
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;       /* Symbol startup code uses to find BSS end */
    } > RAM

    /* Stack lives at the top of RAM, growing downward */
    _estack = ORIGIN(RAM) + LENGTH(RAM);  /* = 0x20018000 */
}