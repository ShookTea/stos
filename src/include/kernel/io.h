#ifndef INCLUDE_KERNEL_IO_H
#define INCLUDE_KERNEL_IO_H

#include <stdint.h>

// Input byte from port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Output byte to port
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %b0, %w1" : : "a"(val), "Nd"(port));
}

// Input word (16-bit) from port
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Output word to port
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %w0, %w1" : : "a"(val), "Nd"(port));
}

// Input long (32-bit) from port
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %w1, %w0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Output long to port
static inline void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %w0, %w1" : : "a"(val), "Nd"(port));
}

// I/O wait (for port sequencing)
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif
