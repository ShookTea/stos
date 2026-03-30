#if !(defined(__is_libk))

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

void* calloc(size_t count, size_t size)
{
    size_t final_size = count * size;
    if (count != 0 && final_size / count != size) {
        // Overflow handling
        return NULL;
    }

    void* space = malloc(final_size);
    memset(space, 0, final_size);
    return space;
}

#endif
