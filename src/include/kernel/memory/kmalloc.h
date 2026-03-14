#ifndef KERNEL_MEMORY_KMALLOC_H
#define KERNEL_MEMORY_KMALLOC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Kernel Dynamic Memory Allocation
 * 
 * Provides malloc-like interface for kernel memory allocation.
 * 
 * Implementation strategy:
 * - Small allocations (<= 2KB): Use slab allocator
 * - Large allocations (> 2KB): Use vmm_kernel_alloc directly
 * 
 * All allocations are tracked to enable proper cleanup with kfree()
 */

// Maximum size for slab allocations
#define KMALLOC_SLAB_THRESHOLD 2048

// Allocation flags
#define KMALLOC_ZERO    0x01  // Zero-initialize memory (like calloc)
#define KMALLOC_ATOMIC  0x02  // Non-blocking allocation (future use)

/**
 * Allocation header for large allocations (> 2KB)
 * Stored just before the returned pointer
 */
typedef struct kmalloc_header {
    uint32_t magic;         // Magic number for validation
    uint32_t size;          // Actual allocation size
    uint32_t flags;         // Allocation flags
} kmalloc_header_t;

// Magic number for allocation headers
#define KMALLOC_MAGIC 0xDEADBEEF

/**
 * Statistics for kmalloc/kfree
 */
typedef struct {
    size_t total_allocated;         // Total bytes allocated
    size_t total_freed;             // Total bytes freed
    size_t current_usage;           // Current usage in bytes
    size_t peak_usage;              // Peak usage in bytes
    size_t num_small_allocations;   // Number of slab allocations
    size_t num_large_allocations;   // Number of large allocations
    size_t num_small_frees;         // Number of slab frees
    size_t num_large_frees;         // Number of large frees
    size_t num_failed_allocations;  // Number of failed allocations
} kmalloc_stats_t;

/**
 * Initialize the kernel memory allocator
 * Must be called after slab_init()
 */
void kmalloc_init(void);

/**
 * Allocate kernel memory
 * 
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* kmalloc(size_t size);

/**
 * Allocate and zero-initialize kernel memory
 * Equivalent to kmalloc with KMALLOC_ZERO flag
 * 
 * @param nmemb Number of members
 * @param size Size of each member in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void* kcalloc(size_t nmemb, size_t size);

/**
 * Allocate kernel memory with flags
 * 
 * @param size Size in bytes to allocate
 * @param flags Allocation flags (KMALLOC_ZERO, etc.)
 * @return Pointer to allocated memory, or NULL on failure
 */
void* kmalloc_flags(size_t size, uint32_t flags);

/**
 * Free kernel memory
 * 
 * @param ptr Pointer to memory to free (can be NULL)
 */
void kfree(void* ptr);

/**
 * Reallocate kernel memory
 * 
 * Changes the size of the memory block pointed to by ptr.
 * If ptr is NULL, behaves like kmalloc.
 * If new_size is 0, behaves like kfree and returns NULL.
 * 
 * @param ptr Pointer to existing allocation (or NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* krealloc(void* ptr, size_t new_size);

/**
 * Duplicate a string (allocate and copy)
 * 
 * @param str String to duplicate (must be null-terminated)
 * @return Pointer to duplicated string, or NULL on failure
 */
char* kstrdup(const char* str);

/**
 * Duplicate a string with maximum length
 * 
 * @param str String to duplicate (must be null-terminated)
 * @param max Maximum number of characters to copy
 * @return Pointer to duplicated string, or NULL on failure
 */
char* kstrndup(const char* str, size_t max);

/**
 * Get kmalloc statistics
 * 
 * @param stats Pointer to structure to fill with statistics
 */
void kmalloc_get_stats(kmalloc_stats_t* stats);

/**
 * Print kmalloc statistics
 * For debugging purposes
 */
void kmalloc_print_stats(void);

/**
 * Validate kmalloc internal structures
 * Checks for corruption and consistency
 * 
 * @return true if valid, false if corruption detected
 */
bool kmalloc_validate(void);

/**
 * Check if a pointer was allocated by kmalloc
 * 
 * @param ptr Pointer to check
 * @return true if allocated by kmalloc, false otherwise
 */
bool kmalloc_is_valid_ptr(void* ptr);

/**
 * Get the size of an allocation
 * 
 * @param ptr Pointer to allocation
 * @return Size of allocation in bytes, or 0 if invalid
 */
size_t kmalloc_size(void* ptr);

// Convenience macros for type-safe allocation

/**
 * Allocate memory for a single structure
 * Example: my_struct_t* s = kmalloc_struct(my_struct_t);
 */
#define kmalloc_struct(type) \
    ((type*)kmalloc(sizeof(type)))

/**
 * Allocate memory for an array of structures
 * Example: int* arr = kmalloc_array(int, 100);
 */
#define kmalloc_array(type, n) \
    ((type*)kmalloc(sizeof(type) * (n)))

/**
 * Allocate and zero-initialize memory for a single structure
 * Example: my_struct_t* s = kcalloc_struct(my_struct_t);
 */
#define kcalloc_struct(type) \
    ((type*)kcalloc(1, sizeof(type)))

/**
 * Allocate and zero-initialize memory for an array of structures
 * Example: int* arr = kcalloc_array(int, 100);
 */
#define kcalloc_array(type, n) \
    ((type*)kcalloc((n), sizeof(type)))

/**
 * Safely free a pointer and set it to NULL
 * Example: kfree_safe(ptr);
 */
#define kfree_safe(ptr) \
    do { \
        kfree(ptr); \
        (ptr) = NULL; \
    } while(0)

#endif