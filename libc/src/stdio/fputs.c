#if !(defined(__is_libk))

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int fputs(const char* restrict s, FILE* restrict stream)
{
    if (stream == NULL || s == NULL) {
        errno = EINVAL;
        return -1;
    }
    return write(stream->fd, s, strlen(s));
}

#endif
