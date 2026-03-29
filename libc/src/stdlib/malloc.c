#if !(defined(__is_libk))

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memory_definitions.h"

// Addresses to the start and end of the memory
static void* heap_start = NULL;
static void* heap_end = NULL;

/**
 * Find first block that is empty and has at least given size.
 */
static stdlib_mem_alloc_header_t* find_fit(size_t size)
{
    stdlib_mem_alloc_header_t* current = heap_start;
    while (current != NULL) {
        if (current->flags & STDLIB_MEM_ALLOC_FLAGS_PRESENT
            || current->length < size) {
                current = current->next;
        } else {
            return current;
        }
    }
    return NULL;
}

/**
 * Increases the heap by at least the given size, creates a new alloc header
 * for empty area and returns it.
 */
static stdlib_mem_alloc_header_t* extend_heap(size_t size)
{
    size_t total_size = size + sizeof(stdlib_mem_alloc_header_t);
    if (total_size < STDLIB_MEM_ALLOC_PAGE_SIZE) {
        // Increase by at least a full page, to reduce number of syscalls
        total_size = STDLIB_MEM_ALLOC_PAGE_SIZE;
    }
    // Pointer to the beginning of new heap
    void* ptr = sbrk(total_size);
    if (ptr == (void*)-1) {
        return NULL;
    }
    heap_end = ptr + total_size;

    // Initialize new entry
    stdlib_mem_alloc_header_t* entry = ptr;
    entry->flags = 0;
    entry->length = total_size - sizeof(stdlib_mem_alloc_header_t);
    entry->next = NULL;

    if (entry == heap_start) {
        // This was the first created entry - just return it
        return entry;
    }

    // Find the last entry in the list
    stdlib_mem_alloc_header_t* last = heap_start;
    while (last->next != NULL) {
        last = last->next;
    }
    if (last->flags & STDLIB_MEM_ALLOC_FLAGS_PRESENT) {
        // Last entry is allocated - add new entry to the end of queue
        last->next = entry;
        return entry;
    } else {
        // Last entry is not used - coalesce empty entries
        last->length += total_size;
        return last;
    }
}

/**
 * If the empty block has size larger than required and the remainder is large
 * enough to be useful (at least size of header + STDLIB_MEM_ALLOC_MIN_ALLOC),
 * we can split the block into smaller parts. Otherwise splitting doesn't make
 * much sense and we can just use the entire block.
 */
static void split_block(stdlib_mem_alloc_header_t* block, size_t size)
{
    if (block->flags & STDLIB_MEM_ALLOC_FLAGS_PRESENT) {
        // We're not splitting allocated blocks - but that call should never
        // happen.
        puts("Attempted block_split of allocated block.");
        abort();
    }

    size_t remainder = block->length - size;
    size_t min_remainder = sizeof(stdlib_mem_alloc_header_t)
        + STDLIB_MEM_ALLOC_MIN_ALLOC;
    if (remainder < min_remainder) {
        // No need in splitting blocks - remainder will be too small
        return;
    }

    // It's worth it to split the segment into two.
    block->length = size;

    // Address of the new block:
    void* next_block_addr = (void*)(((uint8_t*)block) // current block address
        + sizeof(stdlib_mem_alloc_header_t) // Space for current block's header
        + block->length); // Space for current block's content
    stdlib_mem_alloc_header_t* next_block = next_block_addr;
    next_block->flags = 0;
    next_block->length = remainder - sizeof(stdlib_mem_alloc_header_t);
    next_block->next = block->next;
    block->next = next_block;
}

void* malloc(size_t size)
{
    // Initialize heap with sbrk
    if (heap_start == NULL) {
        heap_start = sbrk(0);
        if (heap_start == (void*)-1) {
            return NULL;
        }

        // Allocate initial heap page, so we have a valid region
        void* initial = sbrk(STDLIB_MEM_ALLOC_PAGE_SIZE);
        if (initial == (void*)-1) {
            return NULL;
        }

        heap_end = initial + STDLIB_MEM_ALLOC_PAGE_SIZE;
        // Initialize first free block
        stdlib_mem_alloc_header_t* first_block = heap_start;
        first_block->flags = 0;
        first_block->length = STDLIB_MEM_ALLOC_PAGE_SIZE -
            sizeof(stdlib_mem_alloc_header_t);
        first_block->next = NULL;
    }

    // Handle zero size
    if (size == 0) {
        return NULL;
    }

    // Handle minimum allocation size
    if (size < STDLIB_MEM_ALLOC_MIN_ALLOC) {
        size = STDLIB_MEM_ALLOC_MIN_ALLOC;
    }

    // Find first empty entry
    stdlib_mem_alloc_header_t* entry = find_fit(size);

    if (entry == NULL) {
        entry = extend_heap(size);
        if (entry == NULL) {
            // We can't increase the heap in any way - returning
            return NULL;
        }
    }

    // Try to split block if necessary
    split_block(entry, size);

    // Mark entry as used and return pointer to data
    entry->flags |= STDLIB_MEM_ALLOC_FLAGS_PRESENT;
    return (void*)(((uint8_t*)entry) + sizeof(stdlib_mem_alloc_header_t));
}

#endif
