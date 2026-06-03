#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compares two addresses byte by byte (each interpreted as unsigned char)
 * for [size] bytes. Returns 0 if they are equal; greater than 0 if value at
 * address_1 is larger, and lesser than 0 if value at address_2 is larger.
 */
int memcmp(const void* address_1, const void* address_2, size_t size);
void* memcpy(void* __restrict dest, const void* __restrict src, size_t size);
void* memmove(void*, const void*, size_t);

/**
 * Finds the start of the first occurrence of the substring `needle` of size
 * `nsize` in the memory area `haystack` of size `hsize`. Returns pointer to
 * the beginning of the substring, or NULL if the substring is not found.
 */
void* memmem(
    const void* haystack,
    size_t hsize,
    const void* needle,
    size_t nsize
);

/**
 * Fills memory from [address] to [address+size] with value [value]
 */
void* memset(void* address, int value, size_t size);

size_t strlen(const char*);
char* strcat(char*, const char*);
char* strcpy(char*, const char*);

/**
 * Copies exactly `n` non-null characters from a null-terminated `source` to
 * `dest`; if `source` doesn't have enough non-null characters, the remainder
 * of `dest` will be padded with zeroes.
 */
char* strncpy(char* dest, const char* source, size_t n);

/**
 * Compares two strings, byte by byte, not more than [size] bytes. Bytes that
 * follow a null byte are not compared. Returns 0 if they are equal; greater
 * than 0 if s1 is larger, and lesser than 0 if s2 is larger.
 */
int strncmp(const char* s1, const char* s2, size_t size);

/**
 * Compares two null-terminated strings. Returns 0 if they are equal; greater
 * than 0 if s1 is larger, and lesser than 0 if s2 is larger.
 */
int strcmp(const char* s1, const char* s2);

/**
 * Calculates the length of the initial segment of s which consists entirely
 * of bytes in accept. Returns the number of bytes in the initial segment.
 */
size_t strspn(const char* s, const char* accept);

/**
 * Locates the first occurrence of c (converted to a char) in the string
 * pointed to by s. The terminating null byte is considered part of the string.
 * Returns a pointer to the located character, or NULL if not found.
 */
char* strchr(const char* s, int c);

/**
 * Breaks a string into a sequence of zero or more nonempty tokens. On the first
 * call, the string to be parsed should be specified in str. In each subsequent
 * call that should parse the same string, str must be NULL. Returns a pointer
 * to the next token, or NULL if there are no more tokens.
 */
char* strtok(char* restrict str, const char* restrict delim);

/**
 * Reentrant version of strtok_r. On the first call to `strtok_r`, `str` should
 * point to the string to be parsed, and the value of `saveptr` is ignored. In
 * subsequent calls `str` should be `NULL` and `saveptr` should be unchanged
 * since the previous call.
 */
char* strtok_r(
    char* restrict str,
    const char* restrict delim,
    char** restrict saveptr
);

/**
 * Finds the first occurrence of the substring `needle` in string `haystack`.
 * The terminating NULL bytes are not compared. Returns pointer to the beginning
 * of the substring, or NULL if the substring is not found. It is equivalent to
 * `memmem(haystack, strlen(haystack), needle, strlen(needle))`.
 */
char *strstr(const char *haystack, const char *needle);

/**
 * Duplicates the `src` string in a newly allocated memory. That memory should
 * be freed after use.
 */
char* strdup(const char* src);

#ifdef __cplusplus
}
#endif

#endif
