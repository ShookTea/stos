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

// Device base type flags (highest two bits)
// Compare (type & PS2_DEVICE_TYPE_MASK) with those values
#define PS2_DEVICE_TYPE_MASK 0xC0
// [00] - not detected
#define PS2_DEVICE_TYPE_NOT_DETECTED 0x00
// [10] - keyboard
#define PS2_DEVICE_TYPE_KEYBOARD 0x80
// [01] - mouse
#define PS2_DEVICE_TYPE_MOUSE 0x40
// [11] - unrecognized device
#define PS2_DEVICE_TYPE_NOT_RECOGNIZED 0xC0

// ===== MOUSES =====
// Standard PS/2 mouse (PS/2 bytes: 0x0000)
#define PS2_DEVICE_MOUSE_STANDARD (PS2_DEVICE_TYPE_MOUSE | 0x00)
// Mouse with scroll wheel (PS/2 bytes: 0x0003)
#define PS2_DEVICE_MOUSE_SCROLLWHEEL (PS2_DEVICE_TYPE_MOUSE | 0x01)
// 5-button mouse (PS/2 bytes: 0x0004)
#define PS2_DEVICE_MOUSE_5BUTTON (PS2_DEVICE_TYPE_MOUSE | 0x02)

// ===== KEYBOARDS =====
// AT keyboard (PS/2 bytes: none)
#define PS2_DEVICE_KEYBOARD_AT (PS2_DEVICE_TYPE_KEYBOARD | 0x00)
// MF2 keyboard (PS/2 bytes: 0xAB83, 0xABC1)
#define PS2_DEVICE_KEYBOARD_MF2 (PS2_DEVICE_TYPE_KEYBOARD | 0x01)
// IBM ThinkPads and many "short" keyboards (PS/2 bytes: 0xAB84)
#define PS2_DEVICE_KEYBOARD_SHORT (PS2_DEVICE_TYPE_KEYBOARD | 0x02)
// NCD N-97 or 122-key host connected keyboard (PS/2 bytes: 0xAB85)
#define PS2_DEVICE_KEYBOARD_NCD_N97 (PS2_DEVICE_TYPE_KEYBOARD | 0x03)
// 122-key keyboard (PS/2 bytes: 0xAB86)
#define PS2_DEVICE_KEYBOARD_122_KEY (PS2_DEVICE_TYPE_KEYBOARD | 0x04)
// Japanese G keyboard (PS/2 bytes: 0xAB90)
#define PS2_DEVICE_KEYBOARD_JAPANESE_G (PS2_DEVICE_TYPE_KEYBOARD | 0x05)
// Japanese P keyboard (PS/2 bytes: 0xAB91)
#define PS2_DEVICE_KEYBOARD_JAPANESE_P (PS2_DEVICE_TYPE_KEYBOARD | 0x06)
// Japanese A keyboard (PS/2 bytes: 0xAB92)
#define PS2_DEVICE_KEYBOARD_JAPANESE_A (PS2_DEVICE_TYPE_KEYBOARD | 0x07)
// NCD Sun layout keyboard (PS/2 bytes: 0xACA1)
#define PS2_DEVICE_KEYBOARD_NCD_SUN (PS2_DEVICE_TYPE_KEYBOARD | 0x08)

uint8_t ps2_get_port1_device_type();
uint8_t ps2_get_port2_device_type();

#endif
