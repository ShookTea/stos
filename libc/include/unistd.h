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

/**
 * Changes the location of the program break (the end of the heap) to [addr].
 *
 * If [addr] is equal to NULL or it's equal to current program break,
 * this function will return the current program break.
 *
 * If [addr] is invalid (below the start of the heap, or above the maximum heap
 * size), this function will return NULL.
 *
 * If [addr] is greater than the current program break, this function will
 * increase the heap size and return the new address of the program break.
 *
 * If [addr] is smaller than the current program break, this function will
 * decrease the heap size and return the new address of the program break.
 */
void* brk(void* addr);

#ifdef __cplusplus
}
#endif

#endif
#endif
