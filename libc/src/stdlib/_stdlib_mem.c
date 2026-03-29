#if !(defined(__is_libk))

#include <stddef.h>
#include <unistd.h>

static void* heap_start = NULL;

void __stdlib_mem_init()
{
    heap_start = sbrk(0);
}

void* __stdlib_mem_get_heap_start()
{
    return heap_start;
}

#endif
