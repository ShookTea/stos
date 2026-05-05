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

static bool in_debug_mode = false;

static void launch_task(const char* name, void (*entrypoint)())
{
    task_t* task = task_create(
        name,
        entrypoint,
        true,
        vfs_get_real_root(),
        vfs_get_real_root()
    );
    scheduler_add_task(task);
}

static void kernel_root_task()
{
    // Initialize drivers for basic devices
    ps2_init();
    keyboard_init();
    ata_init();

    // Initialize filesystem and mount initrd.
    vfs_init();
    initrd_mount();
    device_mount();

    // With filesystem initialized fbcon can now load fonts.
    font_load_psf(FONT_MODE_NORMAL, "/initrd/font_light.psf");
    font_load_psf(FONT_MODE_BOLD, "/initrd/font_bold.psf");

    debug_puts("");
    _debug_puts("=== Kernel Initialization Complete ===");
    _debug_puts("All subsystems initialized and tested successfully");
    _debug_puts("Entering idle loop...\n");

    if (in_debug_mode) {
        launch_task("debugger", debugger_init);
    }

    while (1) {}
}

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

    in_debug_mode = strcmp(multiboot2_get_boot_command_line(), "debug") == 0;

    if (in_debug_mode) {
        // Running tests
        libc_run_all_tests();
        vmm_run_all_tests();
        memory_run_all_tests();
        kmalloc_run_all_tests();
    }

    // Initialize PIT driver and scheduling
    pit_init();
    scheduler_init();
    syscall_init();

    launch_task("kernel", kernel_root_task);

    while (1) {}
}
