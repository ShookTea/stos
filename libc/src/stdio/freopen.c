#if !(defined(__is_libk))

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include "./_stdio_internals.h"

FILE* freopen(
    const char* restrict path,
    const char* restrict mode,
    FILE* restrict stream
) {
    if (path == NULL || mode == NULL || stream == NULL) {
        errno = EINVAL;
        return NULL;
    }

    int flags = __stdio_internals_fcntl_flags_from_mode(mode);
    if (flags == -1) {
        errno = EINVAL;
        return NULL;
    }

    fflush(stream);

    if (close(stream->fd) == -1) {
        // errno is set inside `close`
        return NULL;
    }

    int fd = open(path, flags);
    if (fd == -1) {
        // errno is set inside `open`
        return NULL;
    }

    stream->fd = fd;
    return stream;
}

#endif
