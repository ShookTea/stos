#include <string.h>
#include <stdint.h>

void* memset(void* bufptr, int value, size_t size)
{
    unsigned char* buf = (unsigned char*)bufptr;
    unsigned char b = (unsigned char)value;

    // Head: fill until 4-byte aligned
    while (size > 0 && ((uintptr_t)buf & 3)) {
        *buf++ = b;
        size--;
    }
    // Body: 4 bytes at a time
    uint32_t w = (uint32_t)b | ((uint32_t)b << 8)
               | ((uint32_t)b << 16) | ((uint32_t)b << 24);
    uint32_t* buf32 = (uint32_t*)buf;
    while (size >= 4) {
        *buf32++ = w;
        size -= 4;
    }
    // Tail: remaining bytes
    buf = (unsigned char*)buf32;
    while (size--) {
        *buf++ = b;
    }

    return bufptr;
}
