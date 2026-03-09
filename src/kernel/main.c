#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/tty.h>

#include "multiboot2.h"

#if !defined(__i386__)
#error "This kernel needs to be compiled with a ix86-elf compiler"
#endif


void kernel_main(uint32_t magic, multiboot_info_t* mbi)
{
    tty_initialize();
    tty_writestring("Hello, kernel World!\n");
    //tty_enable_cursor(0, 15);
    //tty_update_cursor(21, 0);
    tty_disable_cursor();

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        tty_writestring("Multiboot2 magic is invalid\n");
        return;
    }
    tty_writestring("Multiboot2 magic is correct\n");

    // The interrupt below will call the interrupt handler
    __asm__ volatile ("int $0x3");
    tty_writestring("this shouldn't be printed\n");
}
