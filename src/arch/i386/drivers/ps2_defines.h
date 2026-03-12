#ifndef ARCH_I386_DRIVERS_PS2_DEFINES
#define ARCH_I386_DRIVERS_PS2_DEFINES

#define PS2_COMMAND_PORT 0x64
#define PS2_STATUS_PORT 0x64
#define PS2_DATA_PORT 0x60

// Status port bits
// bit 0 - if set, output buffer is full
// (must be set before attempting to read from PS2_DATA_PORT)
#define PS2_STATUS_OUTPUT_BUFFER 0x01
// bit 1 - if set, input buffer is full
// (must be clear before sending anything to command or data port)
#define PS2_STATUS_INPUT_BUFFER 0x02

// Controller commands
#define PS2_COM_DISABLE_PORT_1 0xAD
#define PS2_COM_ENABLE_PORT_1 0xAE
#define PS2_COM_DISABLE_PORT_2 0xA7
#define PS2_COM_ENABLE_PORT_2 0xA8

// Returns Controller Configuration Byte (CCB) on data port
#define PS2_COM_READ_CCB 0x20
// Updates CCB with byte sent on data port
#define PS2_COM_SET_CCB 0x60

// Response any other than 0x55 = test failed
#define PS2_COM_TEST_CONTROLLER 0xAA
// Results other than 0x00 for both port1 and port2 = test failed
#define PS2_COM_TEST_PORT_1 0xAB
#define PS2_COM_TEST_PORT_2 0xA9

// Tells the controller that next byte in data port should be sent to the
// second port instead of the first one.
#define PS2_COM_SEND_BYTE_TO_PORT_2 0xD4


// Common commands for devices; they should be sent to the data port.
#define PS2_DEVCOM_ENABLE_SCANNING 0xF4
#define PS2_DEVCOM_DISABLE_SCANNING 0xF5
#define PS2_DEVCOM_IDENTIFY 0xF2
#define PS2_DEVCOM_RESPONSE_ACK 0xFA

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

#endif
