#if !(defined(__is_libk))

#include <errno.h>
#include <stdio.h>

int fseek(FILE* stream, long offset, int whence)
{
    if (stream == NULL) {
        errno = EINVAL;
    }
    int res = lseek(stream->fd, offset, whence);
    if (res == 0) {
        stream->ungetc_buf = EOF;
        stream->read_buf_pos = 0;
        stream->read_buf_len = 0;
    }
    return res;
}

#endif
