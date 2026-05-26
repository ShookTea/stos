#if !(defined(__is_libk))

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "./_stdio_internals.h"

FILE* fdopen(int fd, const char* mode)
{
    int flags = __stdio_internals_fcntl_flags_from_mode(mode);
    if (flags == -1) {
        errno = EINVAL;
        return NULL;
    }

    FILE* res = malloc(sizeof(FILE));
    res->fd = fd;
    res->mode = flags;
    return res;
}

#endif
