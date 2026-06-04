#if !(defined(__is_libk))

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include "./_stdio_internals.h"

FILE* fopen(const char* restrict path, const char* restrict mode)
{
    int flags = __stdio_internals_fcntl_flags_from_mode(mode);
    if (flags == -1) {
        errno = EINVAL;
        return NULL;
    }
    int fd = open(path, flags);
    if (fd == -1) {
        // errno is set inside `open`
        return NULL;
    }

    return fdopen(fd, mode);
}

#endif
