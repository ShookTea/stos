#ifndef ARCH_I386_PAGING_H
#define ARCH_I386_PAGING_H

#include <stdint.h>
#include <stdbool.h>

/**
 * x86 Paging Implementation
 *
 * This module implements 32-bit paging for the i386 architecture.
 *
 * Memory Layout:
 * - Page Directory: 1024 entries, each covering 4MB of virtual memory
 * - Page Table: 1024 entries, each covering 4KB of physical memory
 * - Total addressable: 4GB (1024 * 1024 * 4KB)
 *
 * Virtual Address Breakdown (32 bits):
 * [31:22] - Page Directory Index (10 bits, 0-1023)
 * [21:12] - Page Table Index (10 bits, 0-1023)
 * [11:0]  - Page Offset (12 bits, 0-4095)
 */

// Page size constants
#define PAGE_SIZE 4096
#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024

// Page flags (for both PDE and PTE)
#define PAGE_PRESENT    0x001  // Page is present in memory
#define PAGE_WRITE      0x002  // Page is writable (read-only if not set)
#define PAGE_USER       0x004  // User-accessible (supervisor-only if not set)
#define PAGE_WRITETHROUGH 0x008  // Write-through caching
#define PAGE_CACHE_DISABLE 0x010  // Cache disabled
#define PAGE_ACCESSED   0x020  // Page has been accessed (set by CPU)
#define PAGE_DIRTY      0x040  // Page has been written to (set by CPU, PTE only)
#define PAGE_SIZE_4MB   0x080  // 4MB page size (PDE only)
#define PAGE_GLOBAL     0x100  // Global page (not flushed from TLB, PTE only)

// Common flag combinations
#define PAGE_FLAGS_KERNEL (PAGE_PRESENT | PAGE_WRITE)
#define PAGE_FLAGS_USER   (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)

// Available bits (9-11) for OS use
#define PAGE_COW 0x200 // Bit 9: Page should trigger copy on write

/**
 * Page Table Entry (PTE)
 *
 * 32-bit entry that maps a 4KB virtual page to a physical page frame.
 *
 * Bit Layout:
 * [31:12] - Physical page frame address (20 bits)
 * [11:9]  - Available for OS use (3 bits)
 * [8]     - Global flag
 * [7]     - Page Attribute Table
 * [6]     - Dirty flag (set by CPU on write)
 * [5]     - Accessed flag (set by CPU on access)
 * [4]     - Cache disabled
 * [3]     - Write-through
 * [2]     - User/Supervisor
 * [1]     - Read/Write
 * [0]     - Present
 */
typedef struct {
    uint32_t present    : 1;   // Page present in memory
    uint32_t rw         : 1;   // Read/write permission
    uint32_t user       : 1;   // User/supervisor mode
    uint32_t pwt        : 1;   // Page-level write-through
    uint32_t pcd        : 1;   // Page-level cache disable
    uint32_t accessed   : 1;   // Has been accessed (set by CPU)
    uint32_t dirty      : 1;   // Has been written to (set by CPU)
    uint32_t pat        : 1;   // Page attribute table
    uint32_t global     : 1;   // Global page (TLB)
    uint32_t available  : 3;   // Available for OS use
    uint32_t frame      : 20;  // Physical page frame address (bits 31:12)
} __attribute__((packed)) page_table_entry_t;

/**
 * Page Directory Entry (PDE)
 *
 * 32-bit entry that either:
 * 1. Points to a page table (4KB pages)
 * 2. Directly maps a 4MB page (if PSE bit is set)
 *
 * Bit Layout (for page table pointer):
 * [31:12] - Physical address of page table (20 bits)
 * [11:9]  - Available for OS use (3 bits)
 * [8]     - Global flag (ignored for PDE)
 * [7]     - Page size (0 = 4KB pages, 1 = 4MB page)
 * [6]     - Reserved (must be 0)
 * [5]     - Accessed flag (set by CPU)
 * [4]     - Cache disabled
 * [3]     - Write-through
 * [2]     - User/Supervisor
 * [1]     - Read/Write
 * [0]     - Present
 */
typedef struct {
    uint32_t present    : 1;   // Page table present
    uint32_t rw         : 1;   // Read/write permission
    uint32_t user       : 1;   // User/supervisor mode
    uint32_t pwt        : 1;   // Page-level write-through
    uint32_t pcd        : 1;   // Page-level cache disable
    uint32_t accessed   : 1;   // Has been accessed (set by CPU)
    uint32_t reserved   : 1;   // Reserved (must be 0)
    uint32_t page_size  : 1;   // Page size (0 = 4KB, 1 = 4MB)
    uint32_t ignored    : 1;   // Ignored
    uint32_t available  : 3;   // Available for OS use
    uint32_t frame      : 20;  // Physical address of page table (bits 31:12)
} __attribute__((packed)) page_directory_entry_t;

/**
 * Page Table
 *
 * Contains 1024 page table entries, each mapping a 4KB virtual page.
 * Must be 4KB-aligned.
 */
typedef struct {
    page_table_entry_t entries[PAGE_TABLE_SIZE];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

/**
 * Page Directory
 *
 * Contains 1024 page directory entries, each pointing to a page table
 * or directly mapping a 4MB page.
 * Must be 4KB-aligned.
 */
typedef struct {
    page_directory_entry_t entries[PAGE_DIRECTORY_SIZE];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

/**
 * Global kernel page directory (virtual address)
 * This will be present in all process page directories
 */
extern page_directory_t* kernel_page_directory;

/**
 * Initialize the paging system
 *
 * This function:
 * - Creates the kernel page directory
 * - Identity maps the first 4MB (kernel space)
 * - Sets up proper page flags
 * - Enables paging
 */
void paging_init(void);

/**
 * Map a virtual address to a physical address
 *
 * @param virt Virtual address to map
 * @param phys Physical address to map to
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
 * Flush the Translation Lookaside Buffer (TLB) for a specific page
 *
 * @param virt Virtual address of the page to flush
 */
void paging_flush_tlb_entry(uint32_t virt);

/**
 * Flush the entire TLB by reloading CR3
 */
void paging_flush_tlb(void);

/**
 * Create a new page directory
 *
 * @return Pointer to new page directory, or NULL on failure
 */
page_directory_t* paging_create_directory(void);

/**
 * Clone the kernel page directory for a new process
 * This copies kernel mappings (first 4MB) into a new page directory
 *
 * @return Pointer to new page directory, or NULL on failure
 */
page_directory_t* paging_clone_kernel_directory(void);

/**
 * Get the page directory index from a virtual address
 * (i386-specific - used internally)
 */
static inline uint32_t paging_get_pd_index(uint32_t virt)
{
    return virt >> 22;
}

/**
 * Get the page table index from a virtual address
 * (i386-specific - used internally)
 */
static inline uint32_t paging_get_pt_index(uint32_t virt)
{
    return (virt >> 12) & 0x3FF;
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
