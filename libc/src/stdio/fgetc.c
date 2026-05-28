#if !(defined(__is_libk))

#include <stdio.h>
#include <unistd.h>

int fgetc(FILE* stream)
{
    if (stream == NULL) return EOF;
    if (stream->ungetc_buf != EOF) {
        int c = stream->ungetc_buf;
        stream->ungetc_buf = EOF;
        return c;
    }

    if (stream->read_buf_pos >= stream->read_buf_len) {
        size_t n = read(stream->fd, stream->read_buf, BUFSIZ);
        if (n <= 0) return EOF;
        stream->read_buf_len = (int)n;
        stream->read_buf_pos = 0;
    }
    return (unsigned char)stream->read_buf[stream->read_buf_pos++];
}

#endif
