#ifndef _SYS_UNISTD_H
#define _SYS_UNISTD_H 1
#include <sys/cdefs.h>

#include <stddef.h>
#include <stdint.h>

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

extern char* optarg;
extern int optind, opterr, optopt;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parsing of command line arguments.
 * TODO: write better documentation. For now: https://man7.org/linux/man-pages/man3/getopt.3.html
 */
int getopt(int argc, char *argv[], const char *optstring);

#if !(defined(__is_libk))

/**
 * Writes up to [count] bytes from the buffer to the file referred to by file
 * descriptor fd. On success returns the number of written bytes; on error -1
 * is returned and `errno` is set to one of following values:
 * - EBADF - `fd` is not valid descriptor or is not open for writing
 * - EFAULT - `buffer` is outside accessible address space
 */
int write(int fd, const void* buffer, size_t count);

/**
 * Reads up to [count] bytes from given file descriptor into the buffer starting
 * at [buf].
 * On success, returns the number of read bytes (zero indicates end of file),
 * and the file position is advanced by this number. On error -1 is returned and
 * `errno` is set to one of following values:
 * - EBADF - `fd` is not valid descriptor or is not open for reading
 * - EFAULT - `buffer` is outside accessible address space
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
 *
 * On error, this function will return (void*)-1 and set `errno` to `ENOMEM`.
 */
void* brk(void* addr);

/**
 * Changes the heap size by incrementing the program data space by [increment]
 * bytes. This value can be negative to shrink the heap size.
 *
 * On success, it returns the previous program break, which mean that (in case
 * of increasing the heap) it can be used as a pointer to newly allocated area.
 *
 * On error, this function will return (void*)-1 and set `errno` to `ENOMEM`.
 */
void* sbrk(intptr_t increment);

/**
 * Creates a new process by duplicating the calling proccess. On success, it
 * returns 0 for child process, and PID of the child for parent process. On
 * failure it returns -1.
 */
int fork(void);

/**
 * Stores current working directory in the buffer, and returns the pointer to
 * that buffer. If the absolute pathname of the current working directory,
 * including the terminating NULL character, exceeds `size` bytes, then NULL
 * is returned.
 */
char* getcwd(char buf[], size_t size);

/**
 * Allocates enough memory to fit an absolute path of current working directory,
 * stores that working directory as a null-terminated string, and returns it
 * as a pointer. Caller is responsible for freeing that memory.
 */
char* get_current_dir_name(void);

/**
 * Changes the current working directory of the calling task to one specified
 * in `path`.
 */
int chdir(const char* path);

/**
 * Replace the current process image with the ELF executable at `path`.
 * On success this function does not return; execution resumes at the new
 * program's entry point. On failure returns -1.
 *
 * `argv[]` is an array of pointers to strings passed to a program as its CLI
 * arguments. By convention the first item in this array should contain the
 * filename associated with the file being executed. The `argv[]` array must be
 * terminated by a null pointer.
 *
 * `envp[]` is an array of pointers to strings, conventionally of the form
 * key=value, which are passed as the environment variables of the new program.
 * The `envp[]` array must be terminated by a null pointer.
 */
int execve(const char* path, const char* argv[], const char* envp[]);

/**
 * Replace the current process image with the ELF executable at `path`.
 * On success this function does not return; execution resumes at the new
 * program's entry point. On failure returns -1.
 *
 * `arg` and subsequent ellipses together describe a list of one or more
 * pointers to null-terminated strings that represents arguments passed to the
 * executed program. The first argument, by convention, should point to the
 * filename associated with the file being executed. It has to be terminated
 * with a NULL pointer.
 *
 * Environment variables are inherited from the current process.
 */
int execl(const char* path, const char* arg, ... /*, NULL */);

/**
 * Replace the current process image with the ELF executable at `path`.
 * On success this function does not return; execution resumes at the new
 * program's entry point. On failure returns -1.
 *
 * `arg` and subsequent ellipses together describe a list of one or more
 * pointers to null-terminated strings that represents arguments passed to the
 * executed program. The first argument, by convention, should point to the
 * filename associated with the file being executed. It has to be terminated
 * with a NULL pointer.
 *
 * NULL poointer is followed by a `const char* envp[]`. `envp[]` is an array of
 * pointers to strings, conventionally of the form key=value, which are passed
 * as the environment variables of the new program. The `envp[]` array must be
 * terminated by a null pointer.
 */
int execle(
    const char* path,
    const char* arg,
    ... /* , NULL, const char* envp[] */
);

/**
 * Replace the current process image with the ELF executable at `path`.
 * On success this function does not return; execution resumes at the new
 * program's entry point. On failure returns -1.
 *
 * `argv[]` is an array of pointers to strings passed to a program as its CLI
 * arguments. By convention the first item in this array should contain the
 * filename associated with the file being executed. The `argv[]` array must be
 * terminated by a null pointer.
 *
 * Environment variables are inherited from the current process.
 */
int execv(const char* path, const char* argv[]);

/**
 * Moves the file offset of the open file descriptor `fd` to the argument
 * `offset` according to the value of `whence`:
 * - `SEEK_SET` - the file offset is set to `offset`
 * - `SEEK_CUR` - the file offset is set to current offset + `offset`
 * - `SEEK_END` - the file offset is set to the size of the file + `offset`
 *
 * On success it returns the new offset; on failure returns -1.
 */
int lseek(int fd, int offset, int whence);

#endif // #if !(defined(__is_libk))

#ifdef __cplusplus
}
#endif

#endif
