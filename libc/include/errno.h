#ifndef _ERRNO_H
#define _ERRNO_H 1

#include <sys/cdefs.h>

typedef enum {
    EACCES, // Permission denied
    EBADF, // Invalid file descriptor
    ECHILD, // No child processes
    EFAULT, // Bad address
    EINVAL, // Invalid argument
    EIO, // Input/output error
    ENOENT, // No such file or directory
    ENOEXEC, // Exec format error
    ENOMEM, // Not enough space / cannot allocate memory
    ENOTSUP, // Operation not supported
    ENOTTY, // Inappropriate I/O control operation
} errno_t;

extern int errno;

#endif
