#include <stdint.h>
#include <stdbool.h>
#include <kernel/multiboot2.h>
#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <kernel/memory/vmm.h>
#include <kernel/memory/slab.h>
#include <kernel/memory/kmalloc.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Early Kernel Initialization
 *
 * This function is called from boot.s before kernel_main().
 * It performs critical early initialization that must happen
 * before the kernel proper can run:
 *
 * 1. Physical Memory Manager (PMM) initialization
 * 2. Paging setup and enablement
 *
 * This function is called with:
 * - Stack set up
 * - GDT loaded
 * - IDT loaded
 * - Interrupts disabled
 * - Paging disabled
 *
 * After this function returns:
 * - PMM is ready for allocations
 * - Paging is enabled with identity-mapped first 4MB
 * - System ready for kernel_main()
 *
 * @param magic Multiboot2 magic number
 * @param mbi Pointer to multiboot2 information structure
 */
void early_init(uint32_t magic, multiboot_info_t* mbi)
{
    puts("\n\n=== early_init() ===");
    // Validate multiboot2 magic number
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        puts("GRUB MB2 is invalid");
        abort();
    }
    else {
        puts("GRUB MB2 is valid");
    }
    multiboot2_init(mbi);

    // Initialize Physical Memory Manager
    // This must happen before paging because paging_init() needs
    // to allocate page tables from PMM
    pmm_init();

    // Initialize and enable paging
    // This will:
    // - Create kernel page directory
    // - Identity map first 4MB (0x0 - 0x3FFFFF)
    // - Load page directory into CR3
    // - Enable paging by setting CR0.PG bit
    paging_init();

    // Notify PMM that paging is enabled so it can use PHYS_MAP_BASE
    pmm_set_paging_enabled(true);

    // Initialize Virtual Memory Manager
    // This must happen after paging is enabled because VMM works
    // with the virtual address space
    vmm_init();

    // Enable dynamic allocation for VMM regions
    // This must happen after paging (so we can access physical memory via PHYS_MAP_BASE)
    // but BEFORE slab/kmalloc to avoid circular dependency
    vmm_enable_dynamic_allocation();

    // Initialize Slab Allocator
    // This must happen after VMM is initialized because it uses
    // vmm_kernel_alloc() to allocate memory for slabs
    slab_init();

    // Initialize kmalloc/kfree
    // This must happen after slab allocator is initialized
    kmalloc_init();

    // At this point:
    // - PMM is initialized and ready for allocations
    // - Paging is enabled with identity-mapped first 4MB
    // - VMM is initialized and tracking virtual address space
    // - Slab allocator is ready for sub-page allocations
    // - kmalloc/kfree are ready for use
    // - VMM is using dynamic allocation (not limited to bootstrap pool)
    // - All kernel code, data, stack are accessible
    // - System is ready for kernel_main()
}
