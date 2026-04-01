#ifndef _FCNTL_H
#define _FCNTL_H 1

#if !(defined(__is_libk))

#define O_RDONLY 0x01 // Read-only mode
#define O_WRONLY 0x02 // Write-only mode
#define O_RDWR 0x04 // Read+write mode
#define O_TRUNC 0x08 // Truncate mode (make the file empty if already exists)
#define O_CREAT 0x10 // Create mode (create a new file if it doesn't exit)
#define O_APPEND 0x20 // Append mode (all writes added to the end of file)

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
 */
int open(const char* path, int flags);

/**
 * Closes a file descriptor, so it no longer refers to any file and can be
 * reused. It returns 0 on success, or -1 on error.
 */
int close(int fd);

#ifdef __cplusplus
}
#endif

#endif
#endif
