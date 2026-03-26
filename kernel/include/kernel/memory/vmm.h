#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Virtual memory region flags
#define VMM_PRESENT     0x1     // Region is present
#define VMM_WRITE       0x2     // Region is writable
#define VMM_USER        0x4     // Region is accessible from user mode
#define VMM_KERNEL      0x0     // Region is kernel-only

// Virtual memory layout constants
#define VMM_KERNEL_START    0x00100000  // 1MB - Kernel start
#define VMM_KERNEL_HEAP     0x00400000  // 4MB - Kernel heap start
#define VMM_USER_START      0x40000000  // 1GB - User space start
#define VMM_USER_END        0xC0000000  // 3GB - User space end
#define VMM_KERNEL_END      0xFFFFFFFF  // 4GB - Kernel space end

// Page alignment macros
#define PAGE_SIZE           4096
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr)   (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define IS_PAGE_ALIGNED(addr) (((addr) & (PAGE_SIZE - 1)) == 0)

// Structure to represent a virtual memory region
typedef struct vmm_region {
    uint32_t start;             // Start virtual address
    uint32_t end;               // End virtual address (exclusive)
    uint32_t flags;             // Region flags (present, writable, user, etc.)
    bool is_free;               // Whether this region is free
    struct vmm_region* next;    // Next region in linked list
    struct vmm_region* prev;    // Previous region in linked list
} vmm_region_t;

// VMM statistics structure
typedef struct {
    uint32_t total_virtual_space;   // Total virtual address space
    uint32_t used_virtual_space;    // Used virtual address space
    uint32_t free_virtual_space;    // Free virtual address space
    uint32_t num_regions;           // Total number of regions
    uint32_t num_free_regions;      // Number of free regions
    uint32_t num_used_regions;      // Number of used regions
    uint32_t kernel_heap_start;     // Start of kernel heap
    uint32_t kernel_heap_current;   // Current kernel heap position
} vmm_stats_t;

/**
 * Initialize the Virtual Memory Manager
 * Sets up the initial virtual address space layout
 */
void vmm_init(void);

/**
 * Enable dynamic allocation for VMM regions
 * Must be called after kmalloc_init() to switch from bootstrap pool to kmalloc
 */
void vmm_enable_dynamic_allocation(void);

/**
 * Allocate virtual memory pages
 *
 * @param pages Number of pages to allocate
 * @param flags Flags for the allocated region (VMM_WRITE, VMM_USER, etc.)
 * @return Virtual address of allocated region, or NULL on failure
 */
void* vmm_alloc(size_t pages, uint32_t flags);

/**
 * Free previously allocated virtual memory
 *
 * @param virt Virtual address to free
 * @param pages Number of pages to free
 * @return true on success, false on failure
 */
bool vmm_free(void* virt, size_t pages);

/**
 * Map a region of virtual memory to physical memory
 *
 * @param virt Virtual address (must be page-aligned)
 * @param phys Physical address (must be page-aligned)
 * @param pages Number of pages to map
 * @param flags Page flags (PAGE_PRESENT, PAGE_WRITE, etc.)
 * @return true on success, false on failure
 */
bool vmm_map_region(void* virt, uint32_t phys, size_t pages, uint32_t flags);

/**
 * Unmap a region of virtual memory
 *
 * @param virt Virtual address (must be page-aligned)
 * @param pages Number of pages to unmap
 * @return true on success, false on failure
 */
bool vmm_unmap_region(void* virt, size_t pages);

/**
 * Check if a virtual address range is available
 *
 * @param virt Virtual address to check
 * @param pages Number of pages to check
 * @return true if available, false otherwise
 */
bool vmm_is_range_free(uint32_t virt, size_t pages);

/**
 * Find a free virtual memory region of specified size
 *
 * @param pages Number of pages needed
 * @return Virtual address of free region, or 0 on failure
 */
uint32_t vmm_find_free_region(size_t pages);

/**
 * Get VMM statistics
 *
 * @param stats Pointer to vmm_stats_t structure to fill
 */
void vmm_get_stats(vmm_stats_t* stats);

/**
 * Print VMM statistics and memory map
 * For debugging purposes
 */
void vmm_print_stats(void);

/**
 * Print detailed memory map showing all regions
 * For debugging purposes
 */
void vmm_print_memory_map(void);

/**
 * Allocate kernel heap memory (foundation for kmalloc)
 *
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* vmm_kernel_alloc(size_t size);

/**
 * Free kernel heap memory (foundation for kfree)
 *
 * @param ptr Pointer to memory to free
 * @param size Size in bytes to free
 */
void vmm_kernel_free(void* ptr, size_t size);

#endif
