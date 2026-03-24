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

// Physical memory mapping (architecture-specific values, but interface is common)
#define PHYS_MAP_BASE 0xC0000000  // 3GB - Default for i386
#define PHYS_MAP_SIZE 0x40000000  // 1GB - Default mapping size

// Convert physical address to virtual address (for kernel use)
#define PHYS_TO_VIRT(phys)  ((void*)((uint32_t)(phys) + PHYS_MAP_BASE))

// Convert virtual address back to physical (only for mapped region)
#define VIRT_TO_PHYS(virt)  ((uint32_t)(virt) - PHYS_MAP_BASE)

// Check if a virtual address is in the physical mapping region
#define IS_PHYS_MAPPED(virt) ((uint32_t)(virt) >= PHYS_MAP_BASE)

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
 * Convert a physical address to a virtual address in the mapped region
 * This allows safe access to physical memory after paging is enabled
 *
 * @param phys Physical address
 * @return Virtual address that maps to the physical address
 */
static inline void* paging_phys_to_virt(uint32_t phys)
{
    return PHYS_TO_VIRT(phys);
}

/**
 * Convert a virtual address back to physical (for mapped region only)
 *
 * @param virt Virtual address in the physical mapping region
 * @return Physical address
 */
static inline uint32_t paging_virt_to_phys(void* virt)
{
    return VIRT_TO_PHYS(virt);
}

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

/**
 * Get virtual address of the kernel directory
 */
void* paging_get_kernel_directory();

/**
 * Create a new page directory, copy kernel mappings and handle user space
 * based on usermode flag. Returns virtual address of the new directory.
 */
void* paging_clone_directory(
    void* src,
    bool usermode,
    uint32_t user_stack_base,
    uint32_t user_stack_size
);

/**
 * Handle copy-on-write page fault. When process tries to write to a COW page:
 * 1. Check if the page has the COW flag set
 * 2. Allocate a new physical page
 * 3. Copy contents from shared page to a new page
 * 4. Update PTE to point to a new page and mark as writeable
 * 5. Clear COW flag
 * 6. Flush TLB
 *
 * Return true if COW fault was handled.
 */
bool paging_handle_page_fault_cow(uint32_t faulting_addr);

#endif
