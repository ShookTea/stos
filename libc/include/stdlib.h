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

/**
 * Frees allocated address from the heap. The address must have been returned
 * by malloc() or related functions - otherwise, or if [addr] has already been
 * freed before, the behavior of this function is undefined.
 */
void free(void* addr);

/**
 * Changes the allocation size at pointer [ptr] to a new size [size], and
 * returns a pointer to a newly allocated space. This pointer may or may not
 * be different than [ptr].
 * - If [ptr] is null, then this call is equivalent to malloc(size)
 * - If [ptr] is not null and size=0, then this call is equivalent to free(ptr)
 *
 * If the returned pointer is different than [ptr], then the original [ptr]
 * can be considered "freed" and doesn't need to be freed again. The resulting
 * new pointer has to be freed later.
 */
void* realloc(void* ptr, size_t size);

/**
 * Allocates count * size bytes in the heap and returns pointer to the area
 * in memory.
 *
 * In contrast to malloc(count * size), this function will:
 * 1. Clear the allocated memory to zero
 * 2. Verify multiplication overflow - if multiplication of count*size would
 *    cause an integer overflow, this function will return NULL instead of
 *    allocating an incorrect amount of space.
 */
void* calloc(size_t count, size_t size);

/**
 * Search environment variable list to find a variable with a given name and
 * returns a pointer to a corresponding value string, or NULL if there is no
 * match.
 */
char* getenv(const char* name);

#endif

#ifdef __cplusplus
}
#endif

#endif
