#if !(defined(__is_libk))

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
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

    FILE* res = malloc(sizeof(FILE));
    res->fd = fd;
    res->mode = flags;
    res->read_buf_pos = 0;
    res->read_buf_len = 0;
    res->ungetc_buf = EOF;
    return res;
}

#endif
