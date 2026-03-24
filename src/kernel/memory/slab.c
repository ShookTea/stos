#include <kernel/memory/slab.h>
#include <kernel/memory/vmm.h>
#include <kernel/memory/pmm.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Size classes: 8, 16, 32, 64, 128, 256, 512, 1024 bytes
const size_t SLAB_SIZE_CLASSES[SLAB_NUM_CACHES] = {
    8, 16, 32, 64, 128, 256, 512, 1024
};

// Global array of slab caches, one for each size class
static slab_cache_t slab_caches[SLAB_NUM_CACHES];

// Global statistics
static slab_stats_t global_stats;

// Initialization flag
static bool slab_initialized = false;

/**
 * Calculate how many pages we need for a slab of given object size
 */
static size_t calculate_slab_pages(size_t object_size) {
    // Slab metadata (slab_t) is stored at the beginning
    size_t metadata_size = sizeof(slab_t);

    // We want to fit as many objects as possible in 1-2 pages
    // while leaving room for the metadata
    size_t available_in_one_page = PAGE_SIZE - metadata_size;
    size_t objects_in_one_page = available_in_one_page / object_size;

    // For small objects, use 1 page; for larger objects, might need 2
    if (objects_in_one_page >= 8 || object_size >= 512) {
        return 1;  // One page is sufficient
    } else {
        return 2;  // Use two pages for better utilization
    }
}

/**
 * Calculate how many objects fit in a slab
 */
static size_t calculate_objects_per_slab(size_t object_size, size_t pages) {
    size_t metadata_size = sizeof(slab_t);
    size_t total_size = pages * PAGE_SIZE;
    size_t available_size = total_size - metadata_size;
    return available_size / object_size;
}

/**
 * Initialize a slab cache for a specific object size
 */
static void init_cache(slab_cache_t* cache, size_t object_size) {
    cache->object_size = object_size;
    cache->slab_pages = calculate_slab_pages(object_size);
    cache->objects_per_slab = calculate_objects_per_slab(object_size, cache->slab_pages);

    cache->partial_slabs = NULL;
    cache->full_slabs = NULL;
    cache->empty_slabs = NULL;

    cache->num_slabs = 0;
    cache->num_allocations = 0;
    cache->num_frees = 0;
    cache->num_active_objects = 0;
}

void slab_init(void) {
    if (slab_initialized) {
        return;
    }

    // Initialize all caches
    for (size_t i = 0; i < SLAB_NUM_CACHES; i++) {
        init_cache(&slab_caches[i], SLAB_SIZE_CLASSES[i]);
    }

    // Initialize global statistics
    memset(&global_stats, 0, sizeof(slab_stats_t));

    slab_initialized = true;

    printf("Slab allocator initialized with size classes:");
    for (size_t i = 0; i < SLAB_NUM_CACHES; i++) {
        printf(" %zu", SLAB_SIZE_CLASSES[i]);
    }
    printf(" bytes\n");
}

slab_cache_t* slab_find_cache(size_t size) {
    if (size == 0 || size > SLAB_MAX_SIZE) {
        return NULL;
    }

    // Find the smallest size class that fits the requested size
    for (size_t i = 0; i < SLAB_NUM_CACHES; i++) {
        if (size <= SLAB_SIZE_CLASSES[i]) {
            return &slab_caches[i];
        }
    }

    return NULL;
}

/**
 * Remove a slab from its current list
 */
static void remove_from_list(slab_t* slab, slab_t** list_head) {
    if (slab->prev) {
        slab->prev->next = slab->next;
    } else {
        *list_head = slab->next;
    }

    if (slab->next) {
        slab->next->prev = slab->prev;
    }

    slab->next = NULL;
    slab->prev = NULL;
}

/**
 * Add a slab to the head of a list
 */
static void add_to_list(slab_t* slab, slab_t** list_head) {
    slab->next = *list_head;
    slab->prev = NULL;

    if (*list_head) {
        (*list_head)->prev = slab;
    }

    *list_head = slab;
}

slab_t* slab_create_slab(slab_cache_t* cache) {
    // Allocate pages for the slab
    size_t alloc_size = cache->slab_pages * PAGE_SIZE;
    void* slab_memory = vmm_kernel_alloc(alloc_size);

    if (!slab_memory) {
        return NULL;
    }

    // Initialize slab structure at the beginning of the allocated memory
    slab_t* slab = (slab_t*)slab_memory;
    slab->magic = SLAB_MAGIC;
    slab->next = NULL;
    slab->prev = NULL;
    slab->cache = cache;
    slab->num_objects = cache->objects_per_slab;
    slab->num_free = cache->objects_per_slab;
    slab->object_size = cache->object_size;
    slab->current_list = SLAB_LIST_EMPTY;

    // Objects start after the slab metadata
    slab->objects_start = (void*)((uint8_t*)slab_memory + sizeof(slab_t));

    // Initialize free list - thread all objects together
    slab->free_list = NULL;
    uint8_t* object_ptr = (uint8_t*)slab->objects_start;

    for (size_t i = 0; i < slab->num_objects; i++) {
        slab_free_object_t* free_obj = (slab_free_object_t*)object_ptr;
        free_obj->next = slab->free_list;
        slab->free_list = free_obj;
        object_ptr += cache->object_size;
    }

    // Update statistics
    cache->num_slabs++;
    global_stats.num_slabs_created++;
    global_stats.num_page_allocs += cache->slab_pages;

    // Add to empty slabs list
    add_to_list(slab, &cache->empty_slabs);

    return slab;
}

void slab_destroy_slab(slab_t* slab) {
    if (!slab || slab->magic != SLAB_MAGIC) {
        return;
    }

    slab_cache_t* cache = slab->cache;
    size_t alloc_size = cache->slab_pages * PAGE_SIZE;

    // Remove from whatever list it's in
    if (slab->num_free == 0) {
        remove_from_list(slab, &cache->full_slabs);
    } else if (slab->num_free == slab->num_objects) {
        remove_from_list(slab, &cache->empty_slabs);
    } else {
        remove_from_list(slab, &cache->partial_slabs);
    }

    // Clear magic to prevent use-after-free
    slab->magic = 0;

    // Free memory back to VMM
    vmm_kernel_free(slab, alloc_size);

    // Update statistics
    cache->num_slabs--;
}

void slab_update_lists(slab_t* slab) {
    if (!slab || slab->magic != SLAB_MAGIC) {
        return;
    }

    slab_cache_t* cache = slab->cache;

    // Determine target list based on current state
    slab_list_type_t target_list;
    if (slab->num_free == 0) {
        target_list = SLAB_LIST_FULL;
    } else if (slab->num_free == slab->num_objects) {
        target_list = SLAB_LIST_EMPTY;
    } else {
        target_list = SLAB_LIST_PARTIAL;
    }

    // If already in the correct list, nothing to do
    if (slab->current_list == target_list) {
        return;
    }

    // Remove from current list
    slab_t** current_head;
    switch (slab->current_list) {
        case SLAB_LIST_EMPTY:
            current_head = &cache->empty_slabs;
            break;
        case SLAB_LIST_PARTIAL:
            current_head = &cache->partial_slabs;
            break;
        case SLAB_LIST_FULL:
            current_head = &cache->full_slabs;
            break;
    }
    remove_from_list(slab, current_head);

    // Add to target list
    slab_t** target_head;
    switch (target_list) {
        case SLAB_LIST_EMPTY:
            target_head = &cache->empty_slabs;
            break;
        case SLAB_LIST_PARTIAL:
            target_head = &cache->partial_slabs;
            break;
        case SLAB_LIST_FULL:
            target_head = &cache->full_slabs;
            break;
    }
    add_to_list(slab, target_head);

    // Update tracking field
    slab->current_list = target_list;
}

void* slab_alloc_from_slab(slab_t* slab) {
    if (!slab || slab->magic != SLAB_MAGIC || slab->num_free == 0) {
        return NULL;
    }

    // Get object from free list
    slab_free_object_t* obj = slab->free_list;
    if (!obj) {
        return NULL;
    }

    // Update free list
    slab->free_list = obj->next;
    slab->num_free--;

    return (void*)obj;
}

bool slab_free_to_slab(slab_t* slab, void* ptr) {
    if (!slab || slab->magic != SLAB_MAGIC || !ptr) {
        return false;
    }

    // Check for double-free by traversing free list
    slab_free_object_t* current = slab->free_list;
    while (current) {
        if (current == ptr) {
            printf("ERROR: Double-free detected at %p (slab: %p, cache: %zu bytes)\n",
                   ptr, slab, slab->object_size);
            return false;  // Already in free list!
        }
        current = current->next;
    }

    // Add object back to free list
    slab_free_object_t* obj = (slab_free_object_t*)ptr;
    obj->next = slab->free_list;
    slab->free_list = obj;
    slab->num_free++;
    return true;
}

void* slab_alloc(size_t size) {
    if (!slab_initialized) {
        printf("ERROR: slab_alloc called before slab_init\n");
        return NULL;
    }

    if (size == 0 || size > SLAB_MAX_SIZE) {
        return NULL;
    }

    // Find appropriate cache
    slab_cache_t* cache = slab_find_cache(size);
    if (!cache) {
        return NULL;
    }

    slab_t* slab = NULL;

    // Try to allocate from partial slabs first
    if (cache->partial_slabs) {
        slab = cache->partial_slabs;
    }
    // Then try empty slabs
    else if (cache->empty_slabs) {
        slab = cache->empty_slabs;
    }
    // No suitable slabs, create a new one
    else {
        slab = slab_create_slab(cache);
        if (!slab) {
            return NULL;
        }
    }

    // Allocate object from slab
    void* ptr = slab_alloc_from_slab(slab);

    if (ptr) {
        // Update statistics
        cache->num_allocations++;
        cache->num_active_objects++;
        global_stats.num_allocations++;
        global_stats.total_allocated += cache->object_size;
        global_stats.current_usage += cache->object_size;

        if (global_stats.current_usage > global_stats.peak_usage) {
            global_stats.peak_usage = global_stats.current_usage;
        }

        // Update slab lists
        slab_update_lists(slab);
    }

    return ptr;
}

slab_t* slab_get_slab(void* ptr) {
    if (!ptr) {
        return NULL;
    }

    // Slab metadata is at the beginning of each page-aligned region
    // Round down to page boundary to find the slab structure
    uint32_t addr = (uint32_t)ptr;
    uint32_t slab_addr = addr & ~(PAGE_SIZE - 1);
    slab_t* slab = (slab_t*)slab_addr;

    // Validate magic number
    if (slab->magic != SLAB_MAGIC) {
        return NULL;
    }

    // Verify pointer is within this slab's object area
    uint8_t* obj_start = (uint8_t*)slab->objects_start;
    uint8_t* obj_end = obj_start + (slab->num_objects * slab->object_size);

    if ((uint8_t*)ptr < obj_start || (uint8_t*)ptr >= obj_end) {
        return NULL;
    }

    return slab;
}

bool slab_free(void* ptr) {
    if (!ptr || !slab_initialized) {
        return false;
    }

    // Find the slab this pointer belongs to
    slab_t* slab = slab_get_slab(ptr);

    if (!slab) {
        printf("ERROR: slab_free called with invalid pointer: %#x\n", (uint32_t)ptr);
        return false;
    }

    slab_cache_t* cache = slab->cache;

    // Free object back to slab
    if (!slab_free_to_slab(slab, ptr)) {
        // Double-free detected, don't update statistics
        return false;
    }

    // Update statistics
    cache->num_frees++;
    cache->num_active_objects--;
    global_stats.num_frees++;
    global_stats.total_freed += cache->object_size;
    global_stats.current_usage -= cache->object_size;

    // Update slab lists
    slab_update_lists(slab);

    // Optional: Destroy empty slabs to free memory back to system
    // This is a policy decision - we keep one empty slab per cache
    // to avoid constant allocation/deallocation cycles
    if (slab->num_free == slab->num_objects && cache->empty_slabs != slab) {
        // There's another empty slab, we can destroy this one
        if (cache->empty_slabs && cache->empty_slabs->next == slab) {
            // This is the second empty slab, destroy it
            slab_destroy_slab(slab);
        }
    }

    return true;
}

void slab_get_stats(slab_stats_t* stats) {
    if (!stats) {
        return;
    }

    *stats = global_stats;
}

void slab_print_stats(void) {
    printf("\n=== Slab Allocator Statistics ===\n");
    printf("Total allocated: %zu bytes\n", global_stats.total_allocated);
    printf("Total freed:     %zu bytes\n", global_stats.total_freed);
    printf("Current usage:   %zu bytes\n", global_stats.current_usage);
    printf("Peak usage:      %zu bytes\n", global_stats.peak_usage);
    printf("Allocations:     %zu\n", global_stats.num_allocations);
    printf("Frees:           %zu\n", global_stats.num_frees);
    printf("Active allocs:   %zu\n", global_stats.num_allocations - global_stats.num_frees);
    printf("Slabs created:   %zu\n", global_stats.num_slabs_created);
    printf("Page allocations:%zu\n", global_stats.num_page_allocs);
}

void slab_print_caches(void) {
    printf("\n=== Slab Caches ===\n");

    for (size_t i = 0; i < SLAB_NUM_CACHES; i++) {
        slab_cache_t* cache = &slab_caches[i];

        printf("\nCache[%zu bytes]:\n", cache->object_size);
        printf("  Objects per slab: %zu\n", cache->objects_per_slab);
        printf("  Pages per slab:   %zu\n", cache->slab_pages);
        printf("  Total slabs:      %u\n", cache->num_slabs);
        printf("  Active objects:   %u\n", cache->num_active_objects);
        printf("  Allocations:      %u\n", cache->num_allocations);
        printf("  Frees:            %u\n", cache->num_frees);

        // Count slabs in each list
        uint32_t partial_count = 0;
        uint32_t full_count = 0;
        uint32_t empty_count = 0;

        slab_t* s = cache->partial_slabs;
        while (s) {
            partial_count++;
            s = s->next;
        }

        s = cache->full_slabs;
        while (s) {
            full_count++;
            s = s->next;
        }

        s = cache->empty_slabs;
        while (s) {
            empty_count++;
            s = s->next;
        }

        printf("  Partial slabs:    %u\n", partial_count);
        printf("  Full slabs:       %u\n", full_count);
        printf("  Empty slabs:      %u\n", empty_count);
    }
}

bool slab_validate(void) {
    if (!slab_initialized) {
        printf("ERROR: Slab allocator not initialized\n");
        return false;
    }

    bool valid = true;

    for (size_t i = 0; i < SLAB_NUM_CACHES; i++) {
        slab_cache_t* cache = &slab_caches[i];

        // Validate each slab in each list
        slab_t* lists[] = {cache->partial_slabs, cache->full_slabs, cache->empty_slabs};
        const char* list_names[] = {"partial", "full", "empty"};

        for (int list_idx = 0; list_idx < 3; list_idx++) {
            slab_t* slab = lists[list_idx];
            while (slab) {
                // Check magic number
                if (slab->magic != SLAB_MAGIC) {
                    printf("ERROR: Invalid magic in %s slab of cache %zu\n",
                           list_names[list_idx], cache->object_size);
                    valid = false;
                }

                // Check cache pointer
                if (slab->cache != cache) {
                    printf("ERROR: Invalid cache pointer in slab\n");
                    valid = false;
                }

                // Check free count is reasonable
                if (slab->num_free > slab->num_objects) {
                    printf("ERROR: num_free > num_objects in slab\n");
                    valid = false;
                }

                slab = slab->next;
            }
        }
    }

    if (valid) {
        printf("Slab allocator validation: OK\n");
    }

    return valid;
}
