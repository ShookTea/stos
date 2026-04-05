#include <string.h>
#include <stdint.h>

void* memmove(void* dstptr, const void* srcptr, size_t size)
{
    unsigned char* dst = (unsigned char*)dstptr;
    const unsigned char* src = (const unsigned char*)srcptr;

    if (dst < src) {
        // Head: copy unaligned bytes until dst is 4-byte aligned
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
    } else {
        dst += size;
        src += size;
        // Tail: copy unaligned bytes until dst is 4-byte aligned
        while (size > 0 && ((uintptr_t)dst & 3)) {
            *--dst = *--src;
            size--;
        }
        // Body: 4 bytes at a time (backwards)
        uint32_t* d32 = (uint32_t*)dst;
        const uint32_t* s32 = (const uint32_t*)src;
        while (size >= 4) {
            *--d32 = *--s32;
            size -= 4;
        }
        // Head: remaining bytes
        dst = (unsigned char*)d32;
        src = (const unsigned char*)s32;
        while (size--) {
            *--dst = *--src;
        }
    }

    return dstptr;
}
