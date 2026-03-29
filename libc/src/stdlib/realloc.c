#if !(defined(__is_libk))

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "_stdlib_mem.h"

void* realloc(void* ptr, size_t size)
{
    if (ptr == NULL) {
        return malloc(size);
    }

    if (ptr != NULL && size == 0) {
        free(ptr);
        return NULL;
    }

    stdlib_mem_alloc_header_t* entry = __stdlib_mem_get_entry_of_ptr(ptr);
    if (entry->length >= size) {
        // Requested change in size that still fits in the same block - no need
        // to do anything
        return ptr;
    }

    size_t required_change = size - entry->length;
    size_t required_change_after_inclusion = required_change;
    bool next_entry_inclusion_possible = false;
    bool prev_entry_inclusion_possible = false;

    // Try to check if next entry exists, is empty, and has enough space
    stdlib_mem_alloc_header_t* next = entry->next;
    if (next != NULL && !(next->flags & STDLIB_MEM_ALLOC_FLAGS_PRESENT)) {
        // Next entry is free. If it's total size is less than required change,
        // or if it's slightly more but not enough to create a new empty page,
        // we can just include the total block into current one.
        size_t next_length = next->length + sizeof(stdlib_mem_alloc_header_t);
        size_t min_remainder = sizeof(stdlib_mem_alloc_header_t)
            + STDLIB_MEM_ALLOC_MIN_ALLOC;
        if (next_length < required_change) {
            // Next block is not enough to fully satisfy requirements of current
            // pointer, but can potentially be included.
            next_entry_inclusion_possible = true;
            required_change_after_inclusion -= next_length;
        }
        else if (next_length < (required_change + min_remainder)) {
            // Next pointer is enough to fully satisfy requirements of current
            // pointer, but not enough to create a new entry - just include it
            // and return the same pointer.
            entry->length += next_length;
            entry->next = next->next;
            return ptr;
        } else {
            // The next block is large enough to fill our required space and
            // then to also create a new entry in the memory. We can do it
            // immediately.
            entry->length = size;
            void* new_entry_addr = (void*)(((uint8_t*)entry) //current address
                + sizeof(stdlib_mem_alloc_header_t) // Space for block's header
                + entry->length); // Space for content
            stdlib_mem_alloc_header_t* new_entry = new_entry_addr;
            new_entry->flags = 0;
            new_entry->length = next_length
                - (entry->length + sizeof(stdlib_mem_alloc_header_t));
            new_entry->next = next->next;
            entry->next = new_entry;
            return ptr;
        }
    }

    // Try to check if previous entry is empty and can potentially be used
    stdlib_mem_alloc_header_t* prev = __stdlib_mem_get_previous(entry);
    if (prev != NULL && !(prev->flags & STDLIB_MEM_ALLOC_FLAGS_PRESENT)) {
        size_t prev_length = prev->length + sizeof(stdlib_mem_alloc_header_t);
        size_t min_remainder = sizeof(stdlib_mem_alloc_header_t)
            + STDLIB_MEM_ALLOC_MIN_ALLOC;
        if (prev_length < required_change) {
            // Previous block is not enough to fully satisfy requirements,
            // but can potentially be used.
            prev_entry_inclusion_possible = true;
            required_change_after_inclusion =
                (prev_length > required_change_after_inclusion)
                ? 0 : (required_change_after_inclusion - prev_length);
        } else if (prev_length < (required_change + min_remainder)) {
            // Previous pointer is enough to fully satisfy requirements, but not
            // enough to create a new entry - use it to extend current entry.
            prev->next = entry->next;
            prev->length += prev_length;
            prev->flags |= STDLIB_MEM_ALLOC_FLAGS_PRESENT;
            void* newptr = (void*)((uint8_t*)prev)
                + sizeof(stdlib_mem_alloc_header_t);
            memcpy(newptr, ptr, entry->length);
            return newptr;
        } else {
            // Previous pointer is enough to fully satisfy requirements and to
            // create a new entry - reduce the size of previous pointer and
            // create a new entry.
            prev->length -= required_change;
            void* new_entry_addr = (void*)(((uint8_t*)prev)
                + sizeof(stdlib_mem_alloc_header_t)
                + prev->length);
            stdlib_mem_alloc_header_t* new_entry =
                (stdlib_mem_alloc_header_t*)new_entry_addr;
            new_entry->flags = STDLIB_MEM_ALLOC_FLAGS_PRESENT;
            new_entry->next = entry->next;
            prev->next = new_entry;
            new_entry->length = entry->length + required_change;
            void* newptr = (void*)((uint8_t*)new_entry) +
                sizeof(stdlib_mem_alloc_header_t);
            memcpy(newptr, ptr, entry->length);
            return newptr;
        }
    }

    if (prev_entry_inclusion_possible
        && next_entry_inclusion_possible
        && required_change_after_inclusion == 0
    ) {
        // Both previous and next entries are empty, and while including either
        // of them is not enough, if we merge all of them into one it will be
        // enough.
        prev->next = next->next;
        prev->length = prev->length
            + entry->length
            + next->length
            + 2 * sizeof(stdlib_mem_alloc_header_t);
        prev->flags |= STDLIB_MEM_ALLOC_FLAGS_PRESENT;
        void* newptr = (void*)((uint8_t*)prev)
            + sizeof(stdlib_mem_alloc_header_t);
        memcpy(newptr, ptr, entry->length);
        return newptr;
    }

    // No amount of manipulation with previous/next entries can give us enough
    // memory. We should just allocate a new memory with required size and free
    // current one.
    void* newptr = malloc(size);
    memcpy(newptr, ptr, entry->length);
    free(ptr);
    return newptr;
}

#endif
