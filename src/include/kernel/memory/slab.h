#ifndef KERNEL_MEMORY_SLAB_H
#define KERNEL_MEMORY_SLAB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Slab Allocator
 *
 * Efficient sub-page memory allocation for the kernel.
 * Manages fixed-size object allocations using slab caches.
 *
 * Each size class maintains pools of pre-allocated objects,
 * reducing fragmentation and improving allocation performance.
 */

// Maximum allocation size handled by slab allocator
// Larger allocations go directly to vmm_kernel_alloc()
#define SLAB_MAX_SIZE 2048

// Minimum allocation size (for alignment)
#define SLAB_MIN_SIZE 8

// Number of size classes
#define SLAB_NUM_CACHES 8

// Size classes: 8, 16, 32, 64, 128, 256, 512, 1024, 2048 bytes
// (2048 is actually handled as a special case)
extern const size_t SLAB_SIZE_CLASSES[SLAB_NUM_CACHES];

// Magic number for slab validation
#define SLAB_MAGIC 0x5AB5AB5A

/**
 * Free object within a slab
 * Forms a linked list of free objects
 */
typedef struct slab_free_object {
    struct slab_free_object* next;
} slab_free_object_t;

typedef enum {
    SLAB_LIST_EMPTY,
    SLAB_LIST_PARTIAL,
    SLAB_LIST_FULL
} slab_list_type_t;

/**
 * A slab - represents one or more pages divided into fixed-size objects
 */
typedef struct slab {
    uint32_t magic;                 // Magic number for validation
    struct slab* next;              // Next slab in list
    struct slab* prev;              // Previous slab in list
    struct slab_cache* cache;       // Parent cache
    void* objects_start;            // Start of object area
    slab_free_object_t* free_list;  // Head of free object list
    uint16_t num_objects;           // Total objects in this slab
    uint16_t num_free;              // Number of free objects
    uint16_t object_size;           // Size of each object
    slab_list_type_t current_list;  // Current list type
} slab_t;

/**
 * Slab cache - manages slabs for a specific object size
 */
typedef struct slab_cache {
    size_t object_size;             // Size of objects in this cache
    size_t objects_per_slab;        // Number of objects per slab
    size_t slab_pages;              // Number of pages per slab

    slab_t* partial_slabs;          // Slabs with some free objects
    slab_t* full_slabs;             // Slabs with no free objects
    slab_t* empty_slabs;            // Slabs with all objects free

    // Statistics
    uint32_t num_slabs;             // Total number of slabs
    uint32_t num_allocations;       // Total allocations from this cache
    uint32_t num_frees;             // Total frees to this cache
    uint32_t num_active_objects;    // Currently allocated objects
} slab_cache_t;

/**
 * Global slab allocator statistics
 */
typedef struct {
    size_t total_allocated;         // Total bytes allocated
    size_t total_freed;             // Total bytes freed
    size_t current_usage;           // Current usage in bytes
    size_t peak_usage;              // Peak usage in bytes
    size_t num_allocations;         // Total number of allocations
    size_t num_frees;               // Total number of frees
    size_t num_slabs_created;       // Total slabs created
    size_t num_page_allocs;         // Total page allocations from VMM
} slab_stats_t;

/**
 * Initialize the slab allocator
 * Must be called after VMM initialization
 */
void slab_init(void);

/**
 * Allocate memory from slab allocator
 *
 * @param size Size in bytes to allocate (must be <= SLAB_MAX_SIZE)
 * @return Pointer to allocated memory, or NULL on failure
 */
void* slab_alloc(size_t size);

/**
 * Free slab-allocated memory
 *
 * @param ptr Pointer to memory to free (must be slab-allocated)
 * @return true if freed successfully, false if double-free detected
 */
bool slab_free(void* ptr);

/**
 * Get the slab that contains a given pointer
 *
 * @param ptr Pointer to check
 * @return Pointer to slab structure, or NULL if invalid
 */
slab_t* slab_get_slab(void* ptr);

/**
 * Get slab allocator statistics
 *
 * @param stats Pointer to structure to fill with statistics
 */
void slab_get_stats(slab_stats_t* stats);

/**
 * Print slab allocator statistics
 * For debugging purposes
 */
void slab_print_stats(void);

/**
 * Print detailed information about all slab caches
 * For debugging purposes
 */
void slab_print_caches(void);

/**
 * Validate slab allocator integrity
 * Checks magic numbers and internal consistency
 *
 * @return true if valid, false if corruption detected
 */
bool slab_validate(void);

// Internal functions (not for external use)

/**
 * Find the appropriate cache for a given size
 *
 * @param size Allocation size in bytes
 * @return Pointer to cache, or NULL if size too large
 */
slab_cache_t* slab_find_cache(size_t size);

/**
 * Create a new slab for a cache
 *
 * @param cache Cache to create slab for
 * @return Pointer to new slab, or NULL on failure
 */
slab_t* slab_create_slab(slab_cache_t* cache);

/**
 * Destroy a slab and return its memory to VMM
 *
 * @param slab Slab to destroy
 */
void slab_destroy_slab(slab_t* slab);

/**
 * Allocate an object from a specific slab
 *
 * @param slab Slab to allocate from
 * @return Pointer to allocated object, or NULL if slab is full
 */
void* slab_alloc_from_slab(slab_t* slab);

/**
 * Free an object back to its slab
 *
 * @param slab Slab to free to
 * @param ptr Pointer to object to free
 * @return true if freed successfully, false if double-free detected
 */
bool slab_free_to_slab(slab_t* slab, void* ptr);

/**
 * Move a slab between lists (partial/full/empty)
 *
 * @param slab Slab to move
 */
void slab_update_lists(slab_t* slab);

#endif
