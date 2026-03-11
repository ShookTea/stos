#ifndef KERNEL_EARLY_INIT_H
#define KERNEL_EARLY_INIT_H

#include <stdint.h>
#include <kernel/multiboot2.h>

/**
 * Early Kernel Initialization
 * 
 * This function is called from boot.s before kernel_main().
 * It performs critical early initialization:
 * - Physical Memory Manager (PMM) initialization
 * - Paging setup and enablement
 * 
 * Prerequisites (already done by boot.s):
 * - Stack set up
 * - GDT loaded
 * - IDT loaded
 * - Interrupts disabled
 * - Paging disabled
 * 
 * After this function:
 * - PMM is ready for allocations
 * - Paging is enabled with identity-mapped first 4MB
 * - System ready for kernel_main()
 * 
 * @param magic Multiboot2 magic number (passed by bootloader in EAX)
 * @param mbi Pointer to multiboot2 information structure (passed in EBX)
 */
void early_init(uint32_t magic, multiboot_info_t* mbi);

#endif
