#include <libds/libds.h>
#include <stddef.h>
#include <stdlib.h>

static void* (*set_malloc)(size_t) = NULL;
static void* (*set_realloc)(void*, size_t) = NULL;
static void (*set_free)(void*) = NULL;

void* libds_malloc(size_t size)
{
    return (set_malloc == NULL) ? malloc(size) : set_malloc(size);
}

void* libds_realloc(void* ptr, size_t size)
{
    return (set_realloc == NULL) ? realloc(ptr, size) : set_realloc(ptr, size);
}

void libds_free(void* ptr)
{
    (set_free == NULL) ? free(ptr) : set_free(ptr);
}

void libds_set_allocators(
    void* (*f_malloc)(size_t),
    void* (*f_realloc)(void*, size_t),
    void (*f_free)(void*)
) {
    set_malloc = f_malloc;
    set_realloc = f_realloc;
    set_free = f_free;
}
