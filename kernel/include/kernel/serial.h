#ifndef INCLUDE_KERNEL_SERIAL_H
#define INCLUDE_KERNEL_SERIAL_H

#include <stdint.h>

/**
 * Initializes serial port COM1. Returns 1 on error, 0 on success
 */
uint8_t serial_init(void);

/**
 * Send byte to serial
 */
void serial_put_c(uint8_t);

/**
 * Read byte from serial
 */
uint8_t serial_get_c(void);

#endif
