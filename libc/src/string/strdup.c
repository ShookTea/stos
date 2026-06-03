#include <string.h>

#if (defined(__is_libk))
    #include "kernel/memory/kmalloc.h"
#else
    #include <stdlib.h>
#endif

char* strdup(const char* src)
{
    char* res;
    size_t to_alloc = strlen(src) + 1; // reserve space for NULL at the end
    #if (defined(__is_libk))
        res = kmalloc(to_alloc);
    #else
        res = malloc(to_alloc);
    #endif
    strcpy(res, src);
    return res;
}
