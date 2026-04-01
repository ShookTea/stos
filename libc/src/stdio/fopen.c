#if !(defined(__is_libk))

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

static int fcntl_flags_from_mode(const char* restrict mode)
{
    if (strcmp(mode, "r") == 0) {
        return O_RDONLY;
    }
    else if (strcmp(mode, "r+") == 0) {
        return O_RDWR;
    }
    else if (strcmp(mode, "w") == 0) {
        return O_WRONLY | O_CREAT | O_TRUNC;
    }
    else if (strcmp(mode, "w+") == 0) {
        return O_RDWR | O_CREAT | O_TRUNC;
    }
    else if (strcmp(mode, "a") == 0) {
        return O_WRONLY | O_CREAT | O_APPEND;
    }
    else if (strcmp(mode, "a+") == 0) {
        return O_RDWR | O_CREAT | O_APPEND;
    }
    else {
        return -1;
    }
}

FILE* fopen(const char* restrict path, const char* restrict mode)
{
    int flags = fcntl_flags_from_mode(mode);
    if (flags == -1) {
        return NULL;
    }
    int fd = open(path, flags);
    if (fd == -1) {
        return NULL;
    }

    FILE* res = malloc(sizeof(FILE*));
    res->fd = fd;
    res->mode = flags;
    return res;
}

#endif
