#ifndef _LIBDS_LIBDS
#define _LIBDS_LIBDS

#include <stddef.h>

/**
 * Memory allocation function used by the library.
 * Defaults to malloc() from libc, can be overridden.
 */
void* libds_malloc(size_t size);

/**
 * Memory reallocation function used by the library.
 * Defaults to realloc() from libc, can be overridden.
 */
void* libds_realloc(void* ptr, size_t size);

/**
 * Memory freeing function used by the library.
 * Defaults to free() from libc, can be overridden.
 */
void libds_free(void* ptr);

void libds_set_allocators(
    void* (*f_malloc)(size_t),
    void* (*f_realloc)(void*, size_t),
    void (*f_free)(void*)
);

#endif
