#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/multiboot2.h>

// Page size is 4KB (4096 bytes)
#define PMM_PAGE_SIZE 4096

// Memory below 1MB is reserved for BIOS, VGA, etc.
#define PMM_RESERVED_AREA 0x100000

/**
 * Physical Memory Manager (PMM)
 *
 * This module manages physical memory using a bitmap allocator.
 * Each bit in the bitmap represents one 4KB page frame.
 * Bit = 0: page is free
 * Bit = 1: page is used
 */

/**
 * Initialize the physical memory manager using multiboot2 memory map
 *
 * This function:
 * - Identifies available RAM regions
 * - Sets up the bitmap to track page frames
 * - Marks kernel and reserved regions as used
 */
void pmm_init();

/**
 * Allocate a single physical page frame (4KB)
 *
 * @return Physical address of the allocated page, or 0 if allocation failed
 */
uint32_t pmm_alloc_page(void);

/**
 * Allocate multiple contiguous physical page frames
 *
 * @param count Number of pages to allocate
 * @return Physical address of the first page, or 0 if allocation failed
 */
uint32_t pmm_alloc_pages(size_t count);

/**
 * Free a single physical page frame
 *
 * @param addr Physical address of the page to free (must be page-aligned)
 */
void pmm_free_page(uint32_t addr);

/**
 * Free multiple contiguous physical page frames
 *
 * @param addr Physical address of the first page (must be page-aligned)
 * @param count Number of pages to free
 */
void pmm_free_pages(uint32_t addr, size_t count);

/**
 * Get the total amount of physical memory in bytes
 *
 * @return Total physical memory in bytes
 */
uint32_t pmm_get_total_memory(void);

/**
 * Get the amount of used physical memory in bytes
 *
 * @return Used physical memory in bytes
 */
uint32_t pmm_get_used_memory(void);

/**
 * Get the amount of free physical memory in bytes
 *
 * @return Free physical memory in bytes
 */
uint32_t pmm_get_free_memory(void);

/**
 * Print physical memory statistics to the console
 */
void pmm_print_stats(void);

/**
 * Check if a physical address is page-aligned
 *
 * @param addr Physical address to check
 * @return true if aligned, false otherwise
 */
static inline bool pmm_is_aligned(uint32_t addr)
{
    return (addr & (PMM_PAGE_SIZE - 1)) == 0;
}

/**
 * Align an address up to the next page boundary
 *
 * @param addr Address to align
 * @return Page-aligned address
 */
static inline uint32_t pmm_align_up(uint32_t addr)
{
    return (addr + PMM_PAGE_SIZE - 1) & ~(PMM_PAGE_SIZE - 1);
}

/**
 * Align an address down to the previous page boundary
 *
 * @param addr Address to align
 * @return Page-aligned address
 */
static inline uint32_t pmm_align_down(uint32_t addr)
{
    return addr & ~(PMM_PAGE_SIZE - 1);
}

/**
 * Convert a physical address to a page frame number
 *
 * @param addr Physical address
 * @return Page frame number
 */
static inline uint32_t pmm_addr_to_page(uint32_t addr)
{
    return addr / PMM_PAGE_SIZE;
}

/**
 * Convert a page frame number to a physical address
 *
 * @param page Page frame number
 * @return Physical address
 */
static inline uint32_t pmm_page_to_addr(uint32_t page)
{
    return page * PMM_PAGE_SIZE;
}

#endif
