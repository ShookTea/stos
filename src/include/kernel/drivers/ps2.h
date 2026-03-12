#ifndef KERNEL_DRIVERS_PS2_H
#define KERNEL_DRIVERS_PS2_H

#include <stdbool.h>
#include <stdint.h>

void ps2_init();

/**
 * IO/functions and return types
 */

// Byte was sent correctly
#define PS2_RESPONSE_OK 0
// Selected port is not available
#define PS2_RESPONSE_UNAVAILABLE 1
// Sending byte failed with a 500ms timeout
#define PS2_RESPONSE_TIMEOUT 2

// Functions for sending bytes to selected ports
uint8_t ps2_send_byte_port1(uint8_t byte);
uint8_t ps2_send_byte_port2(uint8_t byte);

/**
 * This will wait with a selected timeout and return one of the PS2_RESPONSE_
 * values, while storing the actual result under given address. There's
 * unfortunately no way of knowing which port returned this value, unless
 * we know for sure that one of them is disabled.
 */
uint8_t ps2_read_byte_to_result(uint8_t* result);

#endif
