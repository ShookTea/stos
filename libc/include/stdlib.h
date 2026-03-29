#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);

/**
 * Converts the initial portion of the string pointed to by str to int.
 * Skips leading whitespace, then parses an optional sign followed by digits.
 * No error detection - returns 0 on invalid input.
 */
int atoi(const char* str);

/**
 * Converts the initial portion of the string pointed to by str to long int
 * according to the given base, which must be between 2 and 36 inclusive,
 * or be the special value 0.
 *
 * The string may begin with arbitrary whitespace, followed by an optional
 * plus or minus sign. If base is 0 or 16, the string may include a "0x"
 * prefix, and the number will be read in base 16; otherwise, if base is 0,
 * the number will be read in base 10 unless it begins with "0", in which
 * case it will be read in base 8.
 *
 * If endptr is not NULL, strtol stores the address of the first invalid
 * character in *endptr. If there were no digits at all, strtol stores the
 * original value of str in *endptr. Returns the converted value or 0 if
 * no conversion could be performed.
 */
long strtol(const char* str, char** endptr, int base);

/**
 * Converts the initial portion of the string pointed to by str to
 * unsigned long int according to the given base (same rules as strtol).
 */
unsigned long strtoul(const char* str, char** endptr, int base);

#if !(defined(__is_libk))

/**
 * Cause normal task termination, with given status code.
 */
__attribute__((__noreturn__))
void exit(int status);

/**
 * Allocates [size] bytes and returns a pointer to allocated memory. The memory
 * is not initialized.
 */
void* malloc(size_t size);

#endif

#ifdef __cplusplus
}
#endif

#endif
