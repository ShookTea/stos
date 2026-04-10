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

/**
 * Definitions for status results of various operations
 */
typedef enum {
    /** Operation was successful */
    DS_SUCCESS = 0,
    /** Pointer to a structure was invalid */
    DS_ERR_INVALID = 1,
    /** Failed to save - container full */
    DS_ERR_FULL = 2,
    /** Failed to read - container empty */
    DS_ERR_EMPTY = 3,
    /** Failed to write - item already exists */
    DS_ERR_DUPLICATE = 4,
    /** Failed to read - item not found */
    DS_ERR_NOT_FOUND = 5
} ds_result_t;

#endif
