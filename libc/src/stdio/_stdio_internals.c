#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#if !(defined(__is_libk))
FILE* stdin;
FILE* stdout;
FILE* stderr;

static FILE _stdin_file  = {
    .fd = STDIN_FILENO,
    .mode = O_RDONLY,
    .ungetc_buf = EOF,
};
static FILE _stdout_file = {
    .fd = STDOUT_FILENO,
    .mode = O_WRONLY,
    .ungetc_buf = EOF,
};
static FILE _stderr_file = {
    .fd = STDERR_FILENO,
    .mode = O_WRONLY,
    .ungetc_buf = EOF,
};
#endif

void __stdio_init(void)
{
    #if !(defined(__is_libk))
        stdin  = &_stdin_file;
        stdout = &_stdout_file;
        stderr = &_stderr_file;
    #endif
}

int __stdio_internals_fcntl_flags_from_mode(const char* restrict mode)
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
