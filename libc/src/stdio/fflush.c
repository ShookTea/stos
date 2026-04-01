#if !(defined(__is_libk))

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int fflush(FILE* stream __attribute__((unused)))
{
    // TODO: implement actual flushing behavior (there's no buffers yet)
    return 0;
}

#endif
