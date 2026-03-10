#include <kernel/serial.h>
#include <stdint.h>
#include <stdbool.h>
#include "io.h"

#define COM1 0x3F8

uint8_t serial_init()
{
    outb(COM1 + 1, 0x00); // Disable IRQ
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1 + 1, 0x00); //                  (hi byte)
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B); // IRQ enabled, RTS/DSR set

    // testing serial chip:
    outb(COM1 + 4, 0x1E); // set in loopback mode
    outb(COM1 + 0, 0xAE); // Send byte 0xAE

    if (inb(COM1 + 0) != 0xAE) {
        // Not the same byte as sent - serial is faulty
        return 1;
    }

    // set in normal (non-loopback) mode
    outb(COM1 + 4, 0x0F);
    return 0;
}

uint8_t serial_is_transmit_empty()
{
    return inb(COM1 + 5) & 0x20;
}

void serial_put_c(uint8_t byte)
{
    // Wait for transmit
    while (serial_is_transmit_empty() == 0);
    outb(COM1, byte);
}

uint8_t serial_is_receive_nonempty()
{
    return inb(COM1 + 5) & 1;
}

uint8_t serial_get_c(void)
{
    while (serial_is_receive_nonempty() == 0);
    return inb(COM1);
}
