#if !(defined(__is_libk))

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>


int fclose(FILE* stream)
{
    if (stream == NULL) {
        return EOF;
    }

    fflush(stream);
    int res = close(stream->fd);
    if (res == 0) {
        return 0;
    }

    return EOF;
}

#endif
