#if !(defined(__is_libk))

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

ssize_t getdelim(
    char** restrict lineptr,
    size_t* restrict n,
    int delimiter,
    FILE* restrict stream
) {
    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    // Allocate an initial buffer if none was provided
    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (*lineptr == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    size_t len = 0;
    int c;

    while ((c = fgetc(stream)) != EOF) {
        // Grow the buffer if needed, leaving room for \0
        if (len + 1 >= *n) {
            size_t new_n = *n * 2;
            char* new_ptr = realloc(*lineptr, new_n);
            if (new_ptr == NULL) {
                errno = ENOMEM;
                return -1;
            }
            *lineptr = new_ptr;
            *n = new_n;
        }

        (*lineptr)[len++] = (char)c;
        if (c == delimiter) {
            break;
        }
    }

    // If len=0, we've either tried to read from end of file or there was an
    // error.
    if (len == 0) {
        return -1;
    }

    // Null-terminate
    (*lineptr)[len] = '\0';
    return len;
}

#endif
