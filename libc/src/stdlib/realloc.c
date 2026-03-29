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

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // The simplest implementation - don't check for possibility of extensions,
    // but just alloc a new space, copy the content, and free the original one.
    stdlib_mem_alloc_header_t* entry = __stdlib_mem_get_entry_of_ptr(ptr);
    void* newptr = malloc(size);
    if (newptr == NULL) {
        return NULL;
    }

    memcpy(
        newptr,
        ptr,
        (size < entry->length) ? size : entry->length
    );

    free(ptr);
    return newptr;
}

#endif
