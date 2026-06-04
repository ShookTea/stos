#include "sys/types.h"
#if !(defined(__is_libk))

#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>

size_t fread(void* restrict ptr, size_t size, size_t n, FILE* restrict stream)
{
    if (ptr == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (size == 0 || n == 0) {
        return 0;
    }

    size_t total = 0;
    uint8_t* curr_pointer = ptr;

    for (size_t i = 0; i < n; i++) {
        ssize_t res = read(stream->fd, curr_pointer, size);
        if (res < 0) {
            return -1;
        }
        curr_pointer += size;
        if (res == size) {
            total++;
        }
    }

    return total;
}

#endif
