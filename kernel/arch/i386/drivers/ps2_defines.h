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

#endif
