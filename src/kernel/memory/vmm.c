#include <kernel/spinlock.h>
#include <kernel/memory/vmm.h>
#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

static spinlock_t vmm_lock = SPINLOCK_INIT;

// Head of the virtual memory region list
static vmm_region_t* region_list = NULL;

// Bootstrap region pool (used during initialization)
#define VMM_BOOTSTRAP_POOL_SIZE 32
static vmm_region_t bootstrap_pool[VMM_BOOTSTRAP_POOL_SIZE];
static size_t bootstrap_pool_index = 0;
static bool use_dynamic_allocation = false;

// Dedicated physical page for dynamic VMM regions (to avoid circular dependency)
// We allocate one physical page and carve out vmm_region_t structures from it
static uint32_t vmm_region_page_phys = 0;
static vmm_region_t* vmm_region_page_virt = NULL;
static size_t vmm_region_page_index = 0;
#define VMM_REGIONS_PER_PAGE (PAGE_SIZE / sizeof(vmm_region_t))

// Free list for reusing destroyed regions
static vmm_region_t* vmm_region_free_list = NULL;

// Statistics tracking
static size_t total_regions_created = 0;
static size_t total_regions_destroyed = 0;
static size_t bootstrap_regions_created = 0;
static size_t dynamic_regions_created = 0;



// Forward declarations for internal functions
static vmm_region_t* vmm_create_region(uint32_t start, uint32_t end, uint32_t flags, bool is_free);
static void vmm_destroy_region(vmm_region_t* region);
static void vmm_insert_region(vmm_region_t* region);
static void vmm_remove_region(vmm_region_t* region);
static vmm_region_t* vmm_find_region(uint32_t virt);
static void vmm_merge_free_regions(void);
static vmm_region_t* vmm_split_region(vmm_region_t* region, uint32_t split_point);

/**
 * Enable dynamic allocation for VMM regions
 * Must be called after paging is enabled (so we can access physical memory)
 */
void vmm_enable_dynamic_allocation(void) {
    if (use_dynamic_allocation) {
        return;  // Already enabled
    }

    printf("[VMM] Enabling dynamic region allocation using dedicated physical page\n");
    printf("[VMM] Bootstrap pool usage: %zu/%d regions\n",
           bootstrap_pool_index, VMM_BOOTSTRAP_POOL_SIZE);
    printf("[VMM] Total regions created so far: %zu (%zu bootstrap)\n",
           total_regions_created, bootstrap_regions_created);

    // Allocate a dedicated physical page for VMM regions
    // This avoids circular dependency: vmm -> kmalloc -> slab -> vmm
    vmm_region_page_phys = pmm_alloc_page();
    if (vmm_region_page_phys == 0) {
        printf("[VMM] ERROR: Failed to allocate physical page for regions!\n");
        return;
    }

    // Map it to virtual address (should already be mapped via PHYS_MAP_BASE)
    vmm_region_page_virt = (vmm_region_t*)PHYS_TO_VIRT(vmm_region_page_phys);
    vmm_region_page_index = 0;

    printf("[VMM] Allocated dedicated page at phys=%#x virt=%#x\n",
           vmm_region_page_phys, (uint32_t)vmm_region_page_virt);
    printf("[VMM] Can fit %zu regions per page\n", VMM_REGIONS_PER_PAGE);

    use_dynamic_allocation = true;
}

/**
 * Initialize the Virtual Memory Manager
 */
void vmm_init(void) {
    printf("[VMM] Initializing Virtual Memory Manager (using bootstrap pool)...\n");

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
}

/**
 * Allocate virtual memory pages
 */
void* vmm_alloc(size_t pages, uint32_t flags) {
    if (pages == 0) {
        return NULL;
    }

    spinlock_acquire(&vmm_lock);
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
                        spinlock_release(&vmm_lock);
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
                        spinlock_release(&vmm_lock);
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
                        spinlock_release(&vmm_lock);
                        return NULL;
                    }
                }

                spinlock_release(&vmm_lock);
                return (void*)alloc_start;
            }
        }
        region = region->next;
    }

    spinlock_release(&vmm_lock);
    return NULL;  // No suitable region found
}

/**
 * Free previously allocated virtual memory
 */
bool vmm_free(void* virt, size_t pages) {
    if (!virt || pages == 0) {
        return false;
    }
    spinlock_acquire(&vmm_lock);

    uint32_t virt_addr = (uint32_t)virt;

    // Check if address is page-aligned
    if (!IS_PAGE_ALIGNED(virt_addr)) {
        spinlock_release(&vmm_lock);
        return false;
    }

    vmm_region_t* region = vmm_find_region(virt_addr);
    if (!region) {
        spinlock_release(&vmm_lock);
        return false;
    }

    // Check if this is a used region
    if (region->is_free) {
        spinlock_release(&vmm_lock);
        return false;  // Already free
    }

    // Check if the size matches
    size_t size = pages * PAGE_SIZE;
    if (region->end - region->start != size) {
        spinlock_release(&vmm_lock);
        return false;  // Size mismatch
    }

    // Unmap pages and free physical memory
    vmm_unmap_region(virt, pages);

    // Mark region as free
    region->is_free = true;

    // Merge with adjacent free regions
    vmm_merge_free_regions();

    spinlock_release(&vmm_lock);
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
    stats->kernel_heap_current = 0;  // No longer using bump pointer

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
    printf("  Kernel Heap Start:   %#x\n", stats.kernel_heap_start);
    printf("  Kernel Heap Current: %#x\n", stats.kernel_heap_current);
    printf("\nRegion Allocation Stats:\n");
    printf("  Total created:       %zu\n", total_regions_created);
    printf("  Total destroyed:     %zu\n", total_regions_destroyed);
    printf("  Currently active:    %zu\n", total_regions_created - total_regions_destroyed);
    printf("  Bootstrap created:   %zu\n", bootstrap_regions_created);
    printf("  Dynamic created:     %zu\n", dynamic_regions_created);
    printf("  Using dynamic alloc: %s\n", use_dynamic_allocation ? "Yes" : "No");
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

    // Align size to page boundary and convert to pages
    size_t aligned_size = PAGE_ALIGN_UP(size);
    size_t pages = aligned_size / PAGE_SIZE;

    // Delegate to vmm_alloc with kernel flags
    return vmm_alloc(pages, VMM_KERNEL | VMM_WRITE);
}

/**
 * Free kernel heap memory (foundation for kfree)
 */
void vmm_kernel_free(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return;
    }

    // Align size to page boundary and convert to pages
    size_t aligned_size = PAGE_ALIGN_UP(size);
    size_t pages = aligned_size / PAGE_SIZE;

    // Delegate to vmm_free
    vmm_free(ptr, pages);
}

/* Internal helper functions */

/**
 * Create a new virtual memory region
 */
static vmm_region_t* vmm_create_region(
    uint32_t start,
    uint32_t end,
    uint32_t flags,
    bool is_free
) {
    vmm_region_t* region;

    if (use_dynamic_allocation) {
        // First, try to reuse a region from the free list
        if (vmm_region_free_list != NULL) {
            region = vmm_region_free_list;
            vmm_region_free_list = (vmm_region_t*)region->next;  // Remove from free list
            dynamic_regions_created++;
        } else {
            // No free regions, allocate from the dedicated page
            if (vmm_region_page_index >= VMM_REGIONS_PER_PAGE) {
                printf(
                    "VMM err: Dedicated region page exhausted (%zu/%zu used)\n",
                    vmm_region_page_index,
                    VMM_REGIONS_PER_PAGE
                );
                puts("Free list is empty, all regions in use");
                printf(
                    "Stats: %zu total regions (%zu dynamic, %zu destroyed)\n",
                    total_regions_created,
                    dynamic_regions_created,
                    total_regions_destroyed
                );
                return NULL;
            }

            region = &vmm_region_page_virt[vmm_region_page_index++];
            dynamic_regions_created++;
        }
    } else {
        // Use bootstrap pool during initialization
        if (bootstrap_pool_index >= VMM_BOOTSTRAP_POOL_SIZE) {
            printf(
                "[VMM] ERROR: Bootstrap pool exhausted (%d regions)!\n",
                VMM_BOOTSTRAP_POOL_SIZE
            );
            puts(
                "Hint: Call vmm_enable_dynamic_allocation() after paging_init()"
            );
            return NULL;
        }
        region = &bootstrap_pool[bootstrap_pool_index++];
        bootstrap_regions_created++;
    }

    total_regions_created++;

    region->start = start;
    region->end = end;
    region->flags = flags;
    region->is_free = is_free;
    region->next = NULL;
    region->prev = NULL;

    return region;
}

/**
 * Destroy a virtual memory region
 */
static void vmm_destroy_region(vmm_region_t* region) {
    if (!region) {
        return;
    }

    // Check if this region is from the bootstrap pool
    bool is_bootstrap = region >= bootstrap_pool
        && region < bootstrap_pool + VMM_BOOTSTRAP_POOL_SIZE;

    // Check if this region is from the dedicated page
    bool is_from_dedicated_page = use_dynamic_allocation
        && region >= vmm_region_page_virt
        && region < vmm_region_page_virt + VMM_REGIONS_PER_PAGE;

    if (is_bootstrap) {
        // Bootstrap regions are never actually freed, just counted
        total_regions_destroyed++;
        return;
    }

    if (is_from_dedicated_page) {
        // Add to free list for reuse
        region->next = (struct vmm_region*)vmm_region_free_list;
        region->prev = NULL;
        vmm_region_free_list = region;
        total_regions_destroyed++;
        return;
    }

    // If we get here, something is wrong
    puts("[VMM] WARNING: Attempted to free region from unknown source");
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
 * Remove a region from the list and destroy it
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

    // Destroy the region structure
    vmm_destroy_region(region);
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
        if (region->is_free
            && region->next->is_free
            && region->end == region->next->start) {
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
