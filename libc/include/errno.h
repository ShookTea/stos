#ifndef _ERRNO_H
#define _ERRNO_H 1

#include <sys/cdefs.h>

// TODO: convert this into #defines
typedef enum {
    EACCES, // Permission denied
    EBADF, // Invalid file descriptor
    ECHILD, // No child processes
    EFAULT, // Bad address
    EINVAL, // Invalid argument
    EIO, // Input/output error
    EISDIR, // File descriptor points to a directory
    ELOOP, // Too many levels of symbolic links
    ENOENT, // No such file or directory
    ENOEXEC, // Exec format error
    ENOMEM, // Not enough space / cannot allocate memory
    ENOTDIR, // Specified path is not a directory
    ENOTSUP, // Operation not supported
    ENOTTY, // Inappropriate I/O control operation
    EPIPE, // Attempting to write to closed pipe
    ESRCH, // No such process
} errno_t;

extern int errno;

#endif
