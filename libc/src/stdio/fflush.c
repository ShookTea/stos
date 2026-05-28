#if !(defined(__is_libk))

#include <stdio.h>
#include <fcntl.h>

int fflush(FILE* stream)
{
    stream->read_buf_pos = stream->read_buf_len;
    return 0;
}

#endif
