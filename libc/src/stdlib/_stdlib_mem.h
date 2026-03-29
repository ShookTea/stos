#if !(defined(__is_libk))

#ifndef _STDLIB_SRC_STDLIB_MEM_H
#define _STDLIB_SRC_STDLIB_MEM_H

#include <stdint.h>
#include <stddef.h>

// Minimal size allocation in bytes
#define STDLIB_MEM_ALLOC_MIN_ALLOC 16

#define STDLIB_MEM_ALLOC_PAGE_SIZE 4096

// Block is allocated
#define STDLIB_MEM_ALLOC_FLAGS_PRESENT 0x01

/**
 * The header structure for each allocation block.
 */
typedef struct mem_alloc_header {
    /**
     * Uses STDLIB_MEM_ALLOC_FLAGS_ values
     */
    uint8_t flags;
    /**
     * Size of block in bytes.
     */
    uint32_t length;
    /**
     * Pointer to next block
     */
    struct mem_alloc_header* next;
} stdlib_mem_alloc_header_t;

static void* heap_start = NULL;

/** Initialize memory data */
void __stdlib_mem_init();
/** Return heap start address */
void* __stdlib_mem_get_heap_start();

#endif
#endif
