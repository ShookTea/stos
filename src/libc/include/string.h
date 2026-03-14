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
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);

/**
 * Fills memory from [address] to [address+size] with value [value]
 */
void* memset(void* address, int value, size_t size);

size_t strlen(const char*);
char* strcat(char*, const char*);
char* strcpy(char*, const char*);

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

#ifdef __cplusplus
}
#endif

#endif
