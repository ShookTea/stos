#ifndef _SYS_UNISTD_H
#define _SYS_UNISTD_H 1
#include <sys/cdefs.h>

#if !(defined(__is_libk))

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Writes up to [count] bytes from the buffer to the file referred to by file
 * descriptor fd. On success returns the number of written bytes; on error -1
 * is returned.
 */
int write(int fd, const void* buffer, size_t count);

/**
 * Returns the process ID of the current process.
 */
uint32_t getpid(void);

/**
 * Returns the process ID of the parent of the current process.
 */
uint32_t getppid(void);

#ifdef __cplusplus
}
#endif

#endif
#endif
