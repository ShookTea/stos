/* 32‑bit i386 ELF */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

/* C entry symbol in your user program */
ENTRY(_start)

/* Where the loader will map the program in virtual memory */
BASE_ADDR = 0x00400000;

SECTIONS
{
    /* Start of the image in virtual address space */
    . = BASE_ADDR;

    /* Text + rodata */
    .text ALIGN(0x1000) :
    {
        *(.text .text.*)
        *(.rodata .rodata.*)
    }

    /* Writable data */
    .data ALIGN(0x1000) :
    {
        *(.data .data.*)
    }

    /* Zero‑initialized data (BSS) */
    .bss ALIGN(0x1000) :
    {
        *(.bss .bss.* COMMON)
    }

    /* Optional: provide symbols for end of segments */
    _end = .;
}
