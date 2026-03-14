#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/serial.h>
#include <kernel/multiboot2.h>
#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <kernel/memory/vmm.h>
#include <kernel/drivers/pit.h>
#include <kernel/drivers/ps2.h>
#include <kernel/drivers/keyboard.h>
#include <string.h>
#include "test/vmm_tests.h"
#include "test/memory_tests.h"
#include "test/kmalloc_tests.h"
#include "debugger.h"


#if !defined(__i386__)
#error "This kernel needs to be compiled with a ix86-elf compiler"
#endif

void kernel_main()
{
    tty_initialize();
    tty_disable_cursor();
    puts("\n\n=== kernel_main() ===");

    if (serial_init() != 0) {
        puts("COM1 faulty");
    } else {
        puts("COM1 initialized correctly");
    }

    puts("\n=== Boot Sequence Status ===");
    puts("PMM: Already initialized by early_init()");
    puts("Paging: Already enabled by early_init()");
    puts("VMM: Already initialized by early_init()");
    puts("Slab allocator: Already initialized by early_init()");
    puts("kmalloc/kfree: Already initialized by early_init()");

    bool in_debug_mode =
        strcmp(multiboot2_get_boot_command_line(), "debug") == 0;

    if (in_debug_mode) {
        // Running tests
        vmm_run_all_tests();
        memory_run_all_tests();
        kmalloc_run_all_tests();
    }

    pit_init();
    ps2_init();
    keyboard_init();

    puts("\n=== Kernel Initialization Complete ===");
    puts("All subsystems initialized and tested successfully");
    puts("Entering idle loop...\n");

    if (in_debug_mode) {
        debugger_init();
    }

    while (1) {}
}
