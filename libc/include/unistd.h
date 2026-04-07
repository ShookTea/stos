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
 * Reads up to [count] bytes from given file descriptor into the buffer starting
 * at [buf].
 * On success, returns the number of read bytes (zero indicates end of file),
 * and the file position is advanced by this number. On error -1 is returned.
 */
int read(int fd, void* buf, size_t count);

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

/**
 * Changes the heap size by incrementing the program data space by [increment]
 * bytes. This value can be negative to shrink the heap size.
 *
 * On success, it returns the previous program break, which mean that (in case
 * of increasing the heap) it can be used as a pointer to newly allocated area.
 *
 * In case of error it will return (void*)-1.
 */
void* sbrk(intptr_t increment);

/**
 * Creates a new process by duplicating the calling proccess. On success, it
 * returns 0 for child process, and PID of the child for parent process. On
 * failure it returns -1.
 */
int fork();

/**
 * Replace the current process image with the ELF executable at [path].
 * On success this function does not return; execution resumes at the new
 * program's entry point. On failure returns -1.
 */
int exec(const char* path);

#ifdef __cplusplus
}
#endif

#endif
#endif
