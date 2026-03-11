#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Architecture-independent paging interface
 * 
 * This header defines the interface for virtual memory management.
 * Architecture-specific implementations are in src/arch/<arch>/paging/
 */

// Page size - typically 4KB on most architectures
#define PAGE_SIZE 4096

// Page flags (architecture-independent)
#define PAGE_PRESENT    0x001  // Page is present in memory
#define PAGE_WRITE      0x002  // Page is writable
#define PAGE_USER       0x004  // User-accessible (supervisor-only if not set)
#define PAGE_ACCESSED   0x020  // Page has been accessed
#define PAGE_DIRTY      0x040  // Page has been written to
#define PAGE_GLOBAL     0x100  // Global page (not flushed from TLB)

// Common flag combinations
#define PAGE_FLAGS_KERNEL (PAGE_PRESENT | PAGE_WRITE)
#define PAGE_FLAGS_USER   (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)

/**
 * Initialize the paging system
 * 
 * This function:
 * - Creates the kernel page directory/tables
 * - Identity maps kernel space
 * - Enables paging
 * 
 * Must be called after PMM initialization.
 */
void paging_init(void);

/**
 * Map a virtual address to a physical address
 * 
 * @param virt Virtual address to map (will be page-aligned)
 * @param phys Physical address to map to (will be page-aligned)
 * @param flags Page flags (PAGE_PRESENT, PAGE_WRITE, PAGE_USER, etc.)
 * @return true on success, false on failure
 */
bool paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);

/**
 * Unmap a virtual address
 * 
 * @param virt Virtual address to unmap
 */
void paging_unmap_page(uint32_t virt);

/**
 * Get the physical address for a virtual address
 * 
 * @param virt Virtual address
 * @return Physical address, or 0 if not mapped
 */
uint32_t paging_get_physical_address(uint32_t virt);

/**
 * Check if a virtual address is mapped
 * 
 * @param virt Virtual address to check
 * @return true if mapped, false otherwise
 */
bool paging_is_mapped(uint32_t virt);

/**
 * Get the page offset from a virtual address
 */
static inline uint32_t paging_get_page_offset(uint32_t virt)
{
    return virt & (PAGE_SIZE - 1);
}

/**
 * Align a virtual address down to page boundary
 */
static inline uint32_t paging_align_down(uint32_t addr)
{
    return addr & ~(PAGE_SIZE - 1);
}

/**
 * Align a virtual address up to page boundary
 */
static inline uint32_t paging_align_up(uint32_t addr)
{
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

/**
 * Check if an address is page-aligned
 */
static inline bool paging_is_aligned(uint32_t addr)
{
    return (addr & (PAGE_SIZE - 1)) == 0;
}

/**
 * Convert a virtual address to a page number
 */
static inline uint32_t paging_addr_to_page(uint32_t addr)
{
    return addr / PAGE_SIZE;
}

/**
 * Convert a page number to a virtual address
 */
static inline uint32_t paging_page_to_addr(uint32_t page)
{
    return page * PAGE_SIZE;
}

/**
 * Debug: Dump page directory information
 * Shows all present page directory entries with their properties
 */
void paging_dump_page_directory(void);

/**
 * Debug: Dump a specific page table
 * Shows all present page table entries for the given page directory index
 * 
 * @param pd_index Page directory index (0-1023)
 */
void paging_dump_page_table(uint32_t pd_index);

/**
 * Validate that critical memory regions are properly identity-mapped
 * Tests important addresses like VGA buffer, kernel space, etc.
 * 
 * @return true if all critical regions are correctly identity-mapped
 */
bool paging_validate_identity_mapping(void);

/**
 * Print statistics about current paging state
 * Shows number of mapped pages, page directory entries, etc.
 */
void paging_print_stats(void);

#endif