#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>

#define EOF (-1)

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
 */
int printf(const char* __restrict, ...);

#ifdef __cplusplus
}
#endif

#endif
