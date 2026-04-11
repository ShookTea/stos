#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/serial.h>
#include <kernel/multiboot2.h>
#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <kernel/memory/vmm.h>
#include <kernel/drivers/pit.h>
#include <kernel/drivers/ps2.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/vfs/initrd.h>
#include <string.h>
#include <kernel/vfs/vfs.h>
#include <kernel/task/scheduler.h>
#include <kernel/task/task.h>
#include "kernel/drivers/ata.h"
#include "kernel/drivers/vga/font.h"
#include "kernel/vfs/device.h"
#include <libds/libds.h>
#include "task/syscall.h"
#include "test/libc_tests.h"
#include "test/vmm_tests.h"
#include "test/memory_tests.h"
#include "test/kmalloc_tests.h"
#include "debugger.h"
#include "kernel/debug.h"
#include <kernel/terminal.h>

#define _debug_puts(...) debug_puts_c("main", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("main", __VA_ARGS__)

#if !defined(__i386__)
#error "This kernel needs to be compiled with a ix86-elf compiler"
#endif

void kernel_main()
{
    terminal_init();
    terminal_disable_cursor();
    _debug_puts("\n\n=== kernel_main() ===");

    if (serial_init() != 0) {
        _debug_puts("COM1 faulty");
    } else {
        _debug_puts("COM1 initialized correctly");
    }

    _debug_puts("\n=== Boot Sequence Status ===");
    _debug_puts("PMM: Already initialized by early_init()");
    _debug_puts("Paging: Already enabled by early_init()");
    _debug_puts("VMM: Already initialized by early_init()");
    _debug_puts("Slab allocator: Already initialized by early_init()");
    _debug_puts("kmalloc/kfree: Already initialized by early_init()");

    // Set allocators for libds library
    libds_set_allocators(kmalloc, krealloc, kfree);

    bool in_debug_mode =
        strcmp(multiboot2_get_boot_command_line(), "debug") == 0;

    if (in_debug_mode) {
        // Running tests
        libc_run_all_tests();
        vmm_run_all_tests();
        memory_run_all_tests();
        kmalloc_run_all_tests();
    }

    // Initialize filesystem and mount initrd.
    vfs_init();
    initrd_mount();
    device_mount();

    // With filesystem initialized fbcon can now load fonts.
    font_load_psf("/initrd/font.psf");

    // Initialize drivers for basic devices
    pit_init();
    ps2_init();
    keyboard_init();
    ata_init();

    // Initialize multitasking scheduler & syscall handling
    scheduler_init();
    syscall_init();

    _debug_puts("\n=== Kernel Initialization Complete ===");
    _debug_puts("All subsystems initialized and tested successfully");
    _debug_puts("Entering idle loop...\n");

    if (in_debug_mode) {
        task_t* debugger = task_create("debugger", debugger_init, true);
        scheduler_add_task(debugger);
    }

    while (1) {}
}
