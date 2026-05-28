#if !(defined(__is_libk))

#include <stdio.h>

int ungetc(int c, FILE* stream)
{
    if (c == EOF || stream == NULL) return EOF;
    stream->ungetc_buf = c;
    return c;
}

#endif
