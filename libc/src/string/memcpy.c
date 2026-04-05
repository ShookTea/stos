#include <string.h>
#include <stdint.h>

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size)
{
    unsigned char* dst = (unsigned char*)dstptr;
    const unsigned char* src = (const unsigned char*)srcptr;

    // Head: copy until dst is 4-byte aligned
    while (size > 0 && ((uintptr_t)dst & 3)) {
        *dst++ = *src++;
        size--;
    }
    // Body: 4 bytes at a time
    uint32_t* d32 = (uint32_t*)dst;
    const uint32_t* s32 = (const uint32_t*)src;
    while (size >= 4) {
        *d32++ = *s32++;
        size -= 4;
    }
    // Tail: remaining bytes
    dst = (unsigned char*)d32;
    src = (const unsigned char*)s32;
    while (size--) {
        *dst++ = *src++;
    }

    return dstptr;
}
