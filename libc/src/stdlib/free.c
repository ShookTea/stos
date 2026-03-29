#if !(defined(__is_libk))

#include <stdlib.h>
#include "_stdlib_mem.h"

void free(void* ptr)
{
    if (ptr == NULL) {
        return;
    }

    // Get entry header
    stdlib_mem_alloc_header_t* entry = __stdlib_mem_get_entry_of_ptr(ptr);

    // Clear present flag
    entry->flags = 0;

    // Coalesce with next entry, if exists and is also free
    if (
        entry->next != NULL
        && !(entry->next->flags & STDLIB_MEM_ALLOC_FLAGS_PRESENT)
    ) {
        stdlib_mem_alloc_header_t* next = entry->next;
        entry->next = next->next;
        entry->length += next->length;
        entry->length += sizeof(stdlib_mem_alloc_header_t);
    }

    // Coalesce with previous entry, if exists and is also free
    stdlib_mem_alloc_header_t* previous = __stdlib_mem_get_previous(entry);
    if (previous != NULL
        && !(previous->flags & STDLIB_MEM_ALLOC_FLAGS_PRESENT)
    ) {
        previous->length += entry->length;
        previous->length += sizeof(stdlib_mem_alloc_header_t);
        previous->next = entry->next;
    }

    // TODO: if current entry was the last one, we can reduce the heap
}

#endif
