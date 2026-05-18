#if !(defined(__is_libk))

#include <unistd.h>
#include <stdio.h>

int vfprintf(FILE* restrict stream, const char* restrict format, va_list list)
{
    return vdprintf(stream->fd, format, list);
}

#endif
