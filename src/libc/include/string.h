#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);

/**
 * Fills memory from [address] to [address+size] with value [value]
 */
void* memset(void* address, int value, size_t size);

size_t strlen(const char*);
char* strcat(char*, const char*);
char* strcpy(char*, const char*);

#ifdef __cplusplus
}
#endif

#endif
