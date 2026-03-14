#include <kernel/memory/vmm.h>
#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <kernel/tty.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

// Head of the virtual memory region list
static vmm_region_t* region_list = NULL;

// Kernel heap tracking
static uint32_t kernel_heap_current = VMM_KERNEL_HEAP;
static uint32_t kernel_heap_end = VMM_USER_START; // Heap grows until user space

// Free list for kernel heap allocations (static pool to avoid allocation)
#define KERNEL_HEAP_MAX_FREE_BLOCKS 64

typedef struct kernel_heap_free_block {
    uint32_t addr;
    size_t size;
    bool in_use;
} kernel_heap_free_block_t;

static kernel_heap_free_block_t kernel_heap_free_blocks[KERNEL_HEAP_MAX_FREE_BLOCKS];
static bool kernel_heap_free_list_initialized = false;

// Forward declarations for internal functions
static vmm_region_t* vmm_create_region(uint32_t start, uint32_t end, uint32_t flags, bool is_free);
static void vmm_insert_region(vmm_region_t* region);
static void vmm_remove_region(vmm_region_t* region);
static vmm_region_t* vmm_find_region(uint32_t virt);
static void vmm_merge_free_regions(void);
static vmm_region_t* vmm_split_region(vmm_region_t* region, uint32_t split_point);

/**
 * Initialize the Virtual Memory Manager
 */
void vmm_init(void) {
    printf("[VMM] Initializing Virtual Memory Manager...\n");

    // Create initial region for kernel (1MB - 4MB, already mapped)
    vmm_region_t* kernel_region = vmm_create_region(
        VMM_KERNEL_START,
        VMM_KERNEL_HEAP,
        VMM_KERNEL | VMM_WRITE | VMM_PRESENT,
        false  // Not free, in use by kernel
    );

    if (!kernel_region) {
        printf("[VMM] ERROR: Failed to create initial kernel region\n");
        return;
    }

    vmm_insert_region(kernel_region);

    // Create initial free region for kernel heap (4MB - 1GB)
    vmm_region_t* heap_region = vmm_create_region(
        VMM_KERNEL_HEAP,
        VMM_USER_START,
        VMM_KERNEL | VMM_WRITE,
        true  // Free, available for allocation
    );

    if (!heap_region) {
        printf("[VMM] ERROR: Failed to create initial heap region\n");
        return;
    }

    vmm_insert_region(heap_region);

    // Create initial free region for user space (1GB - 3GB)
    vmm_region_t* user_region = vmm_create_region(
        VMM_USER_START,
        VMM_USER_END,
        VMM_USER | VMM_WRITE,
        true  // Free, available for allocation
    );

    if (!user_region) {
        printf("[VMM] ERROR: Failed to create initial user region\n");
        return;
    }

    vmm_insert_region(user_region);

    printf("[VMM] Initialization complete\n");
    printf("[VMM] Kernel region: 0x%x - 0x%x\n", VMM_KERNEL_START, VMM_KERNEL_HEAP);
    printf("[VMM] Heap region:   0x%x - 0x%x\n", VMM_KERNEL_HEAP, VMM_USER_START);
    printf("[VMM] User region:   0x%x - 0x%x\n", VMM_USER_START, VMM_USER_END);
    
    // Initialize kernel heap free list
    for (int i = 0; i < KERNEL_HEAP_MAX_FREE_BLOCKS; i++) {
        kernel_heap_free_blocks[i].in_use = false;
    }
    kernel_heap_free_list_initialized = true;
}

/**
 * Allocate virtual memory pages
 */
void* vmm_alloc(size_t pages, uint32_t flags) {
    if (pages == 0) {
        return NULL;
    }

    size_t size = pages * PAGE_SIZE;

    // Find a free region that can accommodate this allocation
    vmm_region_t* region = region_list;
    while (region != NULL) {
        if (region->is_free && (region->end - region->start) >= size) {
            // Check if this region matches the requested flags (kernel vs user)
            bool is_kernel_request = !(flags & VMM_USER);
            bool is_kernel_region = region->start < VMM_USER_START;

            if (is_kernel_request == is_kernel_region) {
                // Found a suitable region
                uint32_t alloc_start = region->start;
                uint32_t alloc_end = alloc_start + size;

                // If this allocation uses the entire region, mark it as used
                if (region->end - region->start == size) {
                    region->is_free = false;
                    region->flags = flags | VMM_PRESENT;
                } else {
                    // Split the region
                    vmm_region_t* allocated = vmm_split_region(region, alloc_end);
                    if (allocated) {
                        allocated->is_free = false;
                        allocated->flags = flags | VMM_PRESENT;
                    } else {
                        return NULL;
                    }
                }

                // Allocate physical pages and map them
                for (size_t i = 0; i < pages; i++) {
                    uint32_t phys = pmm_alloc_page();
                    if (phys == 0) {
                        // Failed to allocate physical memory, unmap what we've done
                        vmm_unmap_region((void*)alloc_start, i);
                        vmm_free((void*)alloc_start, pages);
                        return NULL;
                    }

                    uint32_t page_flags = PAGE_PRESENT | PAGE_WRITE;
                    if (flags & VMM_USER) {
                        page_flags |= PAGE_USER;
                    }

                    if (!paging_map_page(alloc_start + (i * PAGE_SIZE), phys, page_flags)) {
                        // Failed to map, free physical page and clean up
                        pmm_free_page(phys);
                        vmm_unmap_region((void*)alloc_start, i);
                        vmm_free((void*)alloc_start, pages);
                        return NULL;
                    }
                }

                return (void*)alloc_start;
            }
        }
        region = region->next;
    }

    return NULL;  // No suitable region found
}

/**
 * Free previously allocated virtual memory
 */
bool vmm_free(void* virt, size_t pages) {
    if (!virt || pages == 0) {
        return false;
    }

    uint32_t virt_addr = (uint32_t)virt;

    // Check if address is page-aligned
    if (!IS_PAGE_ALIGNED(virt_addr)) {
        return false;
    }

    vmm_region_t* region = vmm_find_region(virt_addr);
    if (!region) {
        return false;
    }

    // Check if this is a used region
    if (region->is_free) {
        return false;  // Already free
    }

    // Check if the size matches
    size_t size = pages * PAGE_SIZE;
    if (region->end - region->start != size) {
        return false;  // Size mismatch
    }

    // Unmap pages and free physical memory
    vmm_unmap_region(virt, pages);

    // Mark region as free
    region->is_free = true;

    // Merge with adjacent free regions
    vmm_merge_free_regions();

    return true;
}

/**
 * Map a region of virtual memory to physical memory
 */
bool vmm_map_region(void* virt, uint32_t phys, size_t pages, uint32_t flags) {
    if (!virt || pages == 0) {
        return false;
    }

    uint32_t virt_addr = (uint32_t)virt;

    // Check alignment
    if (!IS_PAGE_ALIGNED(virt_addr) || !IS_PAGE_ALIGNED(phys)) {
        return false;
    }

    // Map each page
    for (size_t i = 0; i < pages; i++) {
        uint32_t v = virt_addr + (i * PAGE_SIZE);
        uint32_t p = phys + (i * PAGE_SIZE);

        if (!paging_map_page(v, p, flags)) {
            // Failed, unmap what we've done
            for (size_t j = 0; j < i; j++) {
                paging_unmap_page(virt_addr + (j * PAGE_SIZE));
            }
            return false;
        }
    }

    return true;
}

/**
 * Unmap a region of virtual memory
 */
bool vmm_unmap_region(void* virt, size_t pages) {
    if (!virt || pages == 0) {
        return false;
    }

    uint32_t virt_addr = (uint32_t)virt;

    // Check alignment
    if (!IS_PAGE_ALIGNED(virt_addr)) {
        return false;
    }

    // Unmap and free physical pages
    for (size_t i = 0; i < pages; i++) {
        uint32_t v = virt_addr + (i * PAGE_SIZE);
        uint32_t phys = paging_get_physical_address(v);

        if (phys != 0) {
            pmm_free_page(phys);
        }

        paging_unmap_page(v);
    }

    return true;
}

/**
 * Check if a virtual address range is available
 */
bool vmm_is_range_free(uint32_t virt, size_t pages) {
    size_t size = pages * PAGE_SIZE;
    uint32_t end = virt + size;

    vmm_region_t* region = region_list;
    while (region != NULL) {
        if (region->is_free && region->start <= virt && region->end >= end) {
            return true;
        }
        region = region->next;
    }

    return false;
}

/**
 * Find a free virtual memory region of specified size
 */
uint32_t vmm_find_free_region(size_t pages) {
    size_t size = pages * PAGE_SIZE;

    vmm_region_t* region = region_list;
    while (region != NULL) {
        if (region->is_free && (region->end - region->start) >= size) {
            return region->start;
        }
        region = region->next;
    }

    return 0;  // Not found
}

/**
 * Get VMM statistics
 */
void vmm_get_stats(vmm_stats_t* stats) {
    if (!stats) {
        return;
    }

    stats->total_virtual_space = 0;
    stats->used_virtual_space = 0;
    stats->free_virtual_space = 0;
    stats->num_regions = 0;
    stats->num_free_regions = 0;
    stats->num_used_regions = 0;
    stats->kernel_heap_start = VMM_KERNEL_HEAP;
    stats->kernel_heap_current = kernel_heap_current;

    vmm_region_t* region = region_list;
    while (region != NULL) {
        uint32_t region_size = region->end - region->start;
        stats->total_virtual_space += region_size;
        stats->num_regions++;

        if (region->is_free) {
            stats->free_virtual_space += region_size;
            stats->num_free_regions++;
        } else {
            stats->used_virtual_space += region_size;
            stats->num_used_regions++;
        }

        region = region->next;
    }
}

/**
 * Print VMM statistics
 */
void vmm_print_stats(void) {
    vmm_stats_t stats;
    vmm_get_stats(&stats);

    printf("\n=== VMM Statistics ===\n");
    printf("Total Virtual Space: %u MB\n", stats.total_virtual_space / (1024 * 1024));
    printf("Used Virtual Space:  %u MB\n", stats.used_virtual_space / (1024 * 1024));
    printf("Free Virtual Space:  %u MB\n", stats.free_virtual_space / (1024 * 1024));
    printf("Total Regions:       %u\n", stats.num_regions);
    printf("Used Regions:        %u\n", stats.num_used_regions);
    printf("Free Regions:        %u\n", stats.num_free_regions);
    printf("Kernel Heap Start:   %#x\n", stats.kernel_heap_start);
    printf("Kernel Heap Current: %#x\n", stats.kernel_heap_current);
    printf("======================\n\n");
}

/**
 * Print detailed memory map
 */
void vmm_print_memory_map(void) {
    printf("\n=== Virtual Memory Map ===\n");
    printf("%-12s %-12s %-10s %-8s %-10s\n",
           "Start", "End", "Size (KB)", "Flags", "Status");
    printf("-----------------------------------------------------------\n");

    vmm_region_t* region = region_list;
    while (region != NULL) {
        uint32_t size_kb = (region->end - region->start) / 1024;
        const char* status = region->is_free ? "FREE" : "USED";
        const char* mode = (region->flags & VMM_USER) ? "USER" : "KERN";

        printf("%#10x  %#10x  %-10u %-8s %-10s\n",
               region->start, region->end, size_kb,
               mode, status);

        region = region->next;
    }

    printf("==========================\n\n");
}

/**
 * Allocate kernel heap memory (foundation for kmalloc)
 */
void* vmm_kernel_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Align size to page boundary
    size = PAGE_ALIGN_UP(size);
    size_t pages = size / PAGE_SIZE;

    uint32_t alloc_addr = 0;

    // First, try to find a suitable block in the free list
    if (kernel_heap_free_list_initialized) {
        for (int i = 0; i < KERNEL_HEAP_MAX_FREE_BLOCKS; i++) {
            if (kernel_heap_free_blocks[i].in_use && kernel_heap_free_blocks[i].size >= size) {
                // Found a suitable block
                alloc_addr = kernel_heap_free_blocks[i].addr;
                
                // If the block is larger than needed, split it
                if (kernel_heap_free_blocks[i].size > size) {
                    kernel_heap_free_blocks[i].addr += size;
                    kernel_heap_free_blocks[i].size -= size;
                } else {
                    // Exact fit, mark as not in use
                    kernel_heap_free_blocks[i].in_use = false;
                }
                break;
            }
        }
    }

    // If no suitable free block found, allocate from the bump pointer
    if (alloc_addr == 0) {
        // Check if we have enough space in the heap
        if (kernel_heap_current + size > kernel_heap_end) {
            return NULL;  // Heap exhausted
        }
        alloc_addr = kernel_heap_current;
        kernel_heap_current += size;
    }

    // Allocate and map physical pages
    for (size_t i = 0; i < pages; i++) {
        uint32_t phys = pmm_alloc_page();
        if (phys == 0) {
            // Failed, clean up
            for (size_t j = 0; j < i; j++) {
                uint32_t addr = alloc_addr + (j * PAGE_SIZE);
                uint32_t p = paging_get_physical_address(addr);
                if (p != 0) {
                    pmm_free_page(p);
                }
                paging_unmap_page(addr);
            }
            return NULL;
        }

        uint32_t virt = alloc_addr + (i * PAGE_SIZE);
        if (!paging_map_page(virt, phys, PAGE_PRESENT | PAGE_WRITE)) {
            // Failed to map, clean up
            pmm_free_page(phys);
            for (size_t j = 0; j < i; j++) {
                uint32_t addr = alloc_addr + (j * PAGE_SIZE);
                uint32_t p = paging_get_physical_address(addr);
                if (p != 0) {
                    pmm_free_page(p);
                }
                paging_unmap_page(addr);
            }
            return NULL;
        }
    }

    return (void*)alloc_addr;
}

/**
 * Free kernel heap memory (foundation for kfree)
 */
void vmm_kernel_free(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return;
    }

    uint32_t addr = (uint32_t)ptr;

    // Align size to page boundary
    size = PAGE_ALIGN_UP(size);
    size_t pages = size / PAGE_SIZE;
    
    // Free all physical pages and unmap them
    for (size_t i = 0; i < pages; i++) {
        uint32_t virt = addr + (i * PAGE_SIZE);
        uint32_t phys = paging_get_physical_address(virt);

        if (phys != 0) {
            pmm_free_page(phys);
        }

        paging_unmap_page(virt);
    }

    // Add this block to the free list for reuse
    if (kernel_heap_free_list_initialized) {
        for (int i = 0; i < KERNEL_HEAP_MAX_FREE_BLOCKS; i++) {
            if (!kernel_heap_free_blocks[i].in_use) {
                kernel_heap_free_blocks[i].addr = addr;
                kernel_heap_free_blocks[i].size = size;
                kernel_heap_free_blocks[i].in_use = true;
                break;
            }
        }
        // If all free block slots are full, the virtual address space is lost
        // but physical memory is still freed
    }
}

/* Internal helper functions */

/**
 * Create a new virtual memory region
 */
static vmm_region_t* vmm_create_region(uint32_t start, uint32_t end, uint32_t flags, bool is_free) {
    // For now, we'll allocate region structures from a simple pool
    // In a more complete implementation, this would use the kernel heap
    static vmm_region_t region_pool[256];
    static size_t pool_index = 0;

    if (pool_index >= 256) {
        return NULL;  // Pool exhausted
    }

    vmm_region_t* region = &region_pool[pool_index++];
    region->start = start;
    region->end = end;
    region->flags = flags;
    region->is_free = is_free;
    region->next = NULL;
    region->prev = NULL;

    return region;
}

/**
 * Insert a region into the sorted list
 */
static void vmm_insert_region(vmm_region_t* region) {
    if (!region) {
        return;
    }

    // If list is empty, make this the head
    if (region_list == NULL) {
        region_list = region;
        return;
    }

    // Find insertion point (keep list sorted by start address)
    vmm_region_t* current = region_list;
    vmm_region_t* prev = NULL;

    while (current != NULL && current->start < region->start) {
        prev = current;
        current = current->next;
    }

    // Insert at the beginning
    if (prev == NULL) {
        region->next = region_list;
        region_list->prev = region;
        region_list = region;
    } else {
        // Insert in the middle or end
        region->prev = prev;
        region->next = current;
        prev->next = region;
        if (current != NULL) {
            current->prev = region;
        }
    }
}

/**
 * Remove a region from the list
 */
static void vmm_remove_region(vmm_region_t* region) {
    if (!region) {
        return;
    }

    if (region->prev != NULL) {
        region->prev->next = region->next;
    } else {
        region_list = region->next;
    }

    if (region->next != NULL) {
        region->next->prev = region->prev;
    }
}

/**
 * Find a region containing the given virtual address
 */
static vmm_region_t* vmm_find_region(uint32_t virt) {
    vmm_region_t* region = region_list;

    while (region != NULL) {
        if (region->start <= virt && region->end > virt) {
            return region;
        }
        region = region->next;
    }

    return NULL;
}

/**
 * Merge adjacent free regions
 */
static void vmm_merge_free_regions(void) {
    vmm_region_t* region = region_list;

    while (region != NULL && region->next != NULL) {
        // If current and next regions are both free and adjacent
        if (region->is_free && region->next->is_free && region->end == region->next->start) {
            // Merge them
            vmm_region_t* next = region->next;
            region->end = next->end;
            vmm_remove_region(next);
            // Don't advance, check if we can merge with the new next
            continue;
        }
        region = region->next;
    }
}

/**
 * Split a region at the given point
 */
static vmm_region_t* vmm_split_region(vmm_region_t* region, uint32_t split_point) {
    if (!region || split_point <= region->start || split_point >= region->end) {
        return NULL;
    }

    // Create a new region for the second part
    vmm_region_t* second = vmm_create_region(
        split_point,
        region->end,
        region->flags,
        region->is_free
    );

    if (!second) {
        return NULL;
    }

    // Adjust the original region
    region->end = split_point;

    // Insert the second region after the first
    second->next = region->next;
    second->prev = region;
    region->next = second;
    if (second->next != NULL) {
        second->next->prev = second;
    }

    return region;  // Return the first part
}
