#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/serial.h>
#include <kernel/multiboot2.h>

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

    if (serial_init() != 0) {
        puts("COM1 faulty");
    } else {
        puts("COM1 initialized correctly");
    }

    tty_writestring("Multiboot2 magic is correct\n");
    multiboot2_init(mbi);
    multiboot2_print_debug_info();

    while (1) {}

    // The interrupt below will call the interrupt handler
    //puts("End of main kernel function - running int 0x03 for testing");
    //__asm__ volatile ("int $0x3");
    //tty_writestring("this shouldn't be printed\n");
}
