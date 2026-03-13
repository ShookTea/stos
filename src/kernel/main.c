#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/serial.h>
#include <kernel/multiboot2.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/vmm.h>
#include <kernel/drivers/pit.h>
#include <kernel/drivers/ps2.h>
#include <kernel/drivers/keyboard.h>
#include "test/vmm_tests.h"
#include "test/memory_tests.h"
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

    // Running tests
    vmm_run_all_tests();
    memory_run_all_tests();

    pit_init();
    ps2_init();
    keyboard_init();

    puts("\n=== Kernel Initialization Complete ===");
    puts("All subsystems initialized and tested successfully");
    puts("Entering idle loop...\n");

    // TODO: normally it should only be called when running with an argument
    // from GRUB multiboot
    debugger_init();

    while (1) {}
}
