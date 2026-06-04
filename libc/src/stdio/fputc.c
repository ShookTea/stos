#if !(defined(__is_libk))

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int fputc(int c, FILE* stream)
{
    if (stream == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (write(stream->fd, &c, 1) == -1) {
        return EOF;
    }
    return c;
}

#endif
