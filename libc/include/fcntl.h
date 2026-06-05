#ifndef _FCNTL_H
#define _FCNTL_H 1

#include <sys/cdefs.h>

#define O_RDONLY 0x01 // Read-only mode
#define O_WRONLY 0x02 // Write-only mode
#define O_RDWR 0x04 // Read+write mode
#define O_TRUNC 0x08 // Truncate mode (make the file empty if already exists)
#define O_CREAT 0x10 // Create mode (create a new file if it doesn't exit)
#define O_APPEND 0x20 // Append mode (all writes added to the end of file)
#define O_DIRECTORY 0x40 // If file is not a directory, opening will fail

#if !(defined(__is_libk))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a file at specified path, with given flags, and returns a non-negative
 * "file descriptor" that can be used to access the content of the file, or -1
 * on error.
 *
 * The flags start with O_ prefix. One of O_RDONLY, O_WRONLY, or O_RDWR must be
 * included.
 *
 * If `O_CREAT` flag is used, an optional `mode` argument can be used to specify
 * the access mode for the newly created file. (TODO: to be implemented by the
 * kernel)
 *
 * On error -1 is returned and `errno` is set to one of following values:
 * - ENOENT - if there is no file under given path
 * - EIO - on I/O error
 * - EINVAL - given path is a directory, but flags are not read-only
 * - EINVAL - missing exactly one of O_RDONLY, O_WRONLY or O_RDWR flags
 * - ENOTDIR - given path is not a directory, but O_DIRECTORY flag was used
 * - ENOTSUP - on unsupported operation and other unknown errors
 */
int open(const char* path, int flags, ... /* mode_t mode */);

/**
 * Closes a file descriptor, so it no longer refers to any file and can be
 * reused. It returns 0 on success, or -1 on error.
 *
 * On error -1 is returned and `errno` is set to one of following values:
 * - ENOTSUP - on unsupported operation and other unknown errors
 */
int close(int fd);

#ifdef __cplusplus
}
#endif

#endif
#endif
