#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>

#define EOF (-1)

#if !(defined(__is_libk))
/**
 * File stream structure.
 */
typedef struct {
    /** File descriptor ID (from unistd.h) */
    int fd;
    /** Mode used for opening a file (combination of O_ flags from fcntl.h) */
    int mode;
} FILE;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write a single character to the standard output stream. If successful,
 * returns the same character; otherwise returns EOF.
 */
int putchar(int);

/**
 * Writes a null-terminated string + a newline to stdout. Returns the number of
 * bytes written (including the newline), or EOF in case of error.
 */
int puts(const char*);

/**
 * Prints formatted string. Returns the number of characters written, or EOF in
 * case of error.
 *
 * Documentation in the sdtio/vprintf.c file gives more information about format
 * placeholders.
 */
int printf(const char* __restrict, ...);

/**
 * Prints formatted string using va_list argument. Returns the number of
 * characters written, or EOF in case of error.
 *
 * Documentation in the sdtio/vprintf.c file gives more information about format
 * placeholders.
 */
int vprintf(const char* __restrict, va_list list);

#if !(defined(__is_libk))
/**
 * Opens a file at given path, with selected mode, and returns a filestream
 * object.
 *
 * Available modes are:
 * -  "r" - open the file for reading. The stream is positioned at the beginning
 *         of the file.
 * - "r+" - open the file for reading and writing. The stream is positioned at
 *          the beginning of the file.
 * -  "w" - truncate the file to zero length or create the file for writing. The
 *          stream is positioned at the beginning of the file.
 * - "w+" - open the file for reading and writing. The file is created if it
 *          doesn't exist, otherwise it's truncated. The stream is positioned
 *          at the beginning of the file.
 * -  "a" - open the file for appending (writing at the end of the file). The
 *          file is created if it doesn't exist.
 * - "a+" - open the file for reading and appending.
 */
FILE* fopen(const char* restrict path, const char* restrict mode);
#endif

#ifdef __cplusplus
}
#endif

#endif
