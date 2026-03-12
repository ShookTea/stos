#ifndef KERNEL_DRIVERS_PS2_H
#define KERNEL_DRIVERS_PS2_H

#include <stdbool.h>
#include <stdint.h>

void ps2_init();

// Functions for sending bytes to selected ports
uint8_t ps2_send_byte_port1(uint8_t byte);
uint8_t ps2_send_byte_port2(uint8_t byte);
// Byte was sent correctly
#define PS2_SEND_BYTE_RESPONSE_OK 0
// Selected port is not available
#define PS2_SEND_BYTE_RESPONSE_UNAVAILABLE 1
// Sending byte failed with a 500ms timeout
#define PS2_SEND_BYTE_RESPONSE_TIMEOUT 2

#endif
