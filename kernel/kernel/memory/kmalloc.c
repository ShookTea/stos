#include "kernel/spinlock.h"
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/slab.h>
#include <kernel/memory/vmm.h>
#include "kernel/debug.h"
#include <string.h>
#include <stdbool.h>

static spinlock_t kmalloc_lock = SPINLOCK_INIT;

// Global statistics
static kmalloc_stats_t kmalloc_stats;

// Initialization flag
static bool kmalloc_initialized = false;

// Helper to align size for large allocations (include header)
static inline size_t align_large_size(size_t size) {
    size_t total = size + sizeof(kmalloc_header_t);
    return PAGE_ALIGN_UP(total);
}

void kmalloc_init(void) {
    if (kmalloc_initialized) {
        return;
    }

    // Initialize statistics
    memset(&kmalloc_stats, 0, sizeof(kmalloc_stats_t));

    kmalloc_initialized = true;
    debug_printf("kmalloc/kfree initialized (threshold: %d bytes)\n", KMALLOC_SLAB_THRESHOLD);
}

void* kmalloc_flags(size_t size, uint32_t flags) {
    if (!kmalloc_initialized) {
        debug_printf("ERROR: kmalloc called before kmalloc_init\n");
        return NULL;
    }

    if (size == 0) {
        return NULL;
    }

    void* ptr = NULL;

    spinlock_acquire(&kmalloc_lock);

    // Small allocation: use slab allocator
    if (size <= KMALLOC_SLAB_THRESHOLD) {
        ptr = slab_alloc(size);

        if (ptr) {
            // Get the actual slab cache size for statistics
            slab_cache_t* cache = slab_find_cache(size);
            size_t actual_size = cache ? cache->object_size : size;

            kmalloc_stats.num_small_allocations++;
            kmalloc_stats.total_allocated += actual_size;
            kmalloc_stats.current_usage += actual_size;

            if (kmalloc_stats.current_usage > kmalloc_stats.peak_usage) {
                kmalloc_stats.peak_usage = kmalloc_stats.current_usage;
            }

            // Zero-initialize if requested
            if (flags & KMALLOC_ZERO) {
                memset(ptr, 0, actual_size);
            }
        } else {
            kmalloc_stats.num_failed_allocations++;
        }
    }
    // Large allocation: use vmm_kernel_alloc
    else {
        size_t total_size = align_large_size(size);
        void* raw_ptr = vmm_kernel_alloc(total_size);

        if (raw_ptr) {
            // Store header at the beginning
            kmalloc_header_t* header = (kmalloc_header_t*)raw_ptr;
            header->magic = KMALLOC_MAGIC;
            header->size = size;
            header->flags = flags;

            // Return pointer after header
            ptr = (void*)((uint8_t*)raw_ptr + sizeof(kmalloc_header_t));

            kmalloc_stats.num_large_allocations++;
            kmalloc_stats.total_allocated += size;
            kmalloc_stats.current_usage += size;

            if (kmalloc_stats.current_usage > kmalloc_stats.peak_usage) {
                kmalloc_stats.peak_usage = kmalloc_stats.current_usage;
            }

            // Zero-initialize if requested
            if (flags & KMALLOC_ZERO) {
                memset(ptr, 0, size);
            }
        } else {
            kmalloc_stats.num_failed_allocations++;
        }
    }

    spinlock_release(&kmalloc_lock);
    return ptr;
}

void* kmalloc(size_t size) {
    return kmalloc_flags(size, 0);
}

void* kcalloc(size_t nmemb, size_t size) {
    // Check for overflow
    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        return NULL;
    }

    size_t total_size = nmemb * size;
    return kmalloc_flags(total_size, KMALLOC_ZERO);
}

void kfree(void* ptr) {
    if (!ptr || !kmalloc_initialized) {
        return;
    }

    spinlock_acquire(&kmalloc_lock);
    // Check if this is a slab allocation
    slab_t* slab = slab_get_slab(ptr);

    if (slab) {
        // This is a slab allocation
        size_t size = slab->cache->object_size;

        // Only update stats if free was successful (no double-free)
        if (slab_free(ptr)) {
            kmalloc_stats.num_small_frees++;
            kmalloc_stats.total_freed += size;
            if (kmalloc_stats.current_usage >= size) {
                kmalloc_stats.current_usage -= size;
            } else {
                kmalloc_stats.current_usage = 0;
            }
        }
    } else {
        // This should be a large allocation
        // Header is just before the pointer
        kmalloc_header_t* header = (kmalloc_header_t*)((uint8_t*)ptr - sizeof(kmalloc_header_t));

        // Validate header
        if (header->magic != KMALLOC_MAGIC) {
            debug_printf(
                "ERROR: kfree called with invalid pointer: %#x (bad magic)\n",
                (uint32_t)ptr
            );
            spinlock_release(&kmalloc_lock);
            return;
        }

        size_t size = header->size;
        size_t total_size = align_large_size(size);

        // Clear magic to prevent double-free
        header->magic = 0;

        // Free the entire block including header
        vmm_kernel_free((void*)header, total_size);

        kmalloc_stats.num_large_frees++;
        kmalloc_stats.total_freed += size;
        if (kmalloc_stats.current_usage >= size) {
            kmalloc_stats.current_usage -= size;
        } else {
            kmalloc_stats.current_usage = 0;
        }
    }

    spinlock_release(&kmalloc_lock);
}

void* krealloc(void* ptr, size_t new_size) {
    // If ptr is NULL, behave like kmalloc
    if (!ptr) {
        return kmalloc(new_size);
    }

    // If new_size is 0, behave like kfree
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    // Get the old size
    size_t old_size = kmalloc_size(ptr);
    if (old_size == 0) {
        debug_printf("ERROR: krealloc called with invalid pointer\n");
        return NULL;
    }

    // If sizes are similar enough, return the same pointer
    // For slab allocations, check if it fits in the same size class
    slab_t* slab = slab_get_slab(ptr);
    if (slab) {
        size_t actual_size = slab->cache->object_size;
        if (new_size <= actual_size) {
            // Still fits in the same slab object
            return ptr;
        }
    } else {
        // For large allocations, check if we're in the same page-aligned size
        size_t old_aligned = align_large_size(old_size);
        size_t new_aligned = align_large_size(new_size);
        if (old_aligned == new_aligned) {
            // Update the header with new size
            kmalloc_header_t* header = (kmalloc_header_t*)((uint8_t*)ptr - sizeof(kmalloc_header_t));
            header->size = new_size;
            return ptr;
        }
    }

    // Need to allocate new memory
    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data (copy the smaller of old and new size)
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);

    // Free old memory
    kfree(ptr);

    return new_ptr;
}

char* kstrdup(const char* str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    char* dup = (char*)kmalloc(len + 1);

    if (dup) {
        memcpy(dup, str, len + 1);
    }

    return dup;
}

char* kstrndup(const char* str, size_t max) {
    if (!str) {
        return NULL;
    }

    // Find actual length (up to max)
    size_t len = 0;
    while (len < max && str[len] != '\0') {
        len++;
    }

    char* dup = (char*)kmalloc(len + 1);

    if (dup) {
        memcpy(dup, str, len);
        dup[len] = '\0';
    }

    return dup;
}

void kmalloc_get_stats(kmalloc_stats_t* stats) {
    if (!stats) {
        return;
    }

    *stats = kmalloc_stats;
}

void kmalloc_print_stats(void) {
    debug_printf("\n=== kmalloc/kfree Statistics ===\n");
    debug_printf("Total allocated:    %zu bytes\n", kmalloc_stats.total_allocated);
    debug_printf("Total freed:        %zu bytes\n", kmalloc_stats.total_freed);
    debug_printf("Current usage:      %zu bytes\n", kmalloc_stats.current_usage);
    debug_printf("Peak usage:         %zu bytes\n", kmalloc_stats.peak_usage);
    debug_printf("\nSmall allocations:  %zu\n", kmalloc_stats.num_small_allocations);
    debug_printf("Small frees:        %zu\n", kmalloc_stats.num_small_frees);
    debug_printf("Large allocations:  %zu\n", kmalloc_stats.num_large_allocations);
    debug_printf("Large frees:        %zu\n", kmalloc_stats.num_large_frees);
    debug_printf("Failed allocations: %zu\n", kmalloc_stats.num_failed_allocations);
    debug_printf("\nActive allocations: %zu\n",
        (kmalloc_stats.num_small_allocations + kmalloc_stats.num_large_allocations) -
        (kmalloc_stats.num_small_frees + kmalloc_stats.num_large_frees)
    );
}

bool kmalloc_validate(void) {
    if (!kmalloc_initialized) {
        debug_printf("ERROR: kmalloc not initialized\n");
        return false;
    }

    // Validate slab allocator
    if (!slab_validate()) {
        debug_printf("ERROR: Slab allocator validation failed\n");
        return false;
    }

    // Check for consistency in statistics
    if (kmalloc_stats.total_freed > kmalloc_stats.total_allocated) {
        debug_printf("ERROR: More memory freed than allocated\n");
        return false;
    }

    if (kmalloc_stats.current_usage > kmalloc_stats.total_allocated) {
        debug_printf("ERROR: Current usage exceeds total allocated\n");
        return false;
    }

    debug_printf("kmalloc validation: OK\n");
    return true;
}

bool kmalloc_is_valid_ptr(void* ptr) {
    if (!ptr) {
        return false;
    }

    // Check if it's a slab allocation
    slab_t* slab = slab_get_slab(ptr);
    if (slab) {
        return true;
    }

    // Check if it's a large allocation
    kmalloc_header_t* header = (kmalloc_header_t*)((uint8_t*)ptr - sizeof(kmalloc_header_t));

    // Try to validate the header (this is a best-effort check)
    if (header->magic == KMALLOC_MAGIC) {
        return true;
    }

    return false;
}

size_t kmalloc_size(void* ptr) {
    if (!ptr) {
        return 0;
    }

    // Check if it's a slab allocation
    slab_t* slab = slab_get_slab(ptr);
    if (slab) {
        return slab->cache->object_size;
    }

    // Check if it's a large allocation
    kmalloc_header_t* header = (kmalloc_header_t*)((uint8_t*)ptr - sizeof(kmalloc_header_t));

    if (header->magic == KMALLOC_MAGIC) {
        return header->size;
    }

    return 0;
}
