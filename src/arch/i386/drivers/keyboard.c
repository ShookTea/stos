#include "stdlib.h"
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/ps2.h>
#include "../idt/pic.h"
#include "../idt/idt.h"
#include "../io.h"
#include "ps2_defines.h"
#include "keyboard_defines.h"

#include <stdbool.h>
#include <stdio.h>

#define COM_GET_SET_CURRENT_SCAN_CODE_SET 0xF0
#define COM_ENABLE_SCANNING 0xF4

#define RESPONSE_ACK 0xFA
#define RESPONSE_RESEND 0xFE

static bool keyboard_on_port2 = false;
static bool initialized = false;

static uint8_t buffer[6];
static uint8_t buffer_pos = 0;

/**
 * Each command has to receive an ACK for it. It may also receive a "resend"
 * command. We'll keep try up to 3 retries before assuming that the command
 * is not supported and return "false".
 * TODO: better error handling - we probably shouldn't throw abort() here
 */
static bool send_command(uint8_t command)
{
    uint8_t retry = 0, send_result, read_result, read_value;
    while (retry < 3) {
        send_result = keyboard_on_port2
            ? ps2_send_byte_port2(command)
            : ps2_send_byte_port1(command);
        if (send_result == PS2_RESPONSE_TIMEOUT) {
            // Another attempt
            retry++;
        } else if (send_result == PS2_RESPONSE_UNAVAILABLE) {
            // we should probably get some better error handling in this case
            puts("Unexpected PS2_RESPONSE_UNAVAILABLE from the keyboard");
            abort();
        } else if (send_result == PS2_RESPONSE_OK) {
            // Command was delivered. Now let's read the response.
            read_result = ps2_read_byte_to_result(&read_value);
            if (read_result != PS2_RESPONSE_OK) {
                printf(
                    "Unexpected no-response to command %#02x on PS/2 keyboard",
                    command
                );
                abort();
            } else if (read_value == RESPONSE_RESEND) {
                // Let's try again
                retry++;
            } else if (read_value == RESPONSE_ACK) {
                // Success!
                return true;
            } else {
                printf(
                    "Unexpected resp. %#02x to command %#02x on PS/2 keyboard",
                    read_value,
                    command
                );
            }
        } else {
            printf("Unknown send_result code %#02x\n", send_result);
            abort();
        }
    }

    // After 3 tries, no success
    return false;
}

/**
 * TODO: maybe instead of panic we should handle it more gracefully?
 */
static void buffer_dump_and_panic()
{
    puts("Unrecognized PS/2 input buffer:");
    for (int i = 0; i < buffer_pos; i++) {
        printf("%#02x ", buffer[i]);
    }
    buffer_pos = 0;
    abort();
}

// This is called when receiving a keyboard interrupt
static void irq_callback()
{

    // In interrupt context, we cannot use timeouts (PIT cannot fire while
    // interrupts are disabled) - we must check if data is available immediately
    // and read it without waiting
    if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER) {
        uint8_t value = inb(PS2_DATA_PORT);
        buffer[buffer_pos] = value;
        if (buffer_pos == 0) {
            if (value == KEYBOARD_RELEASE_BYTE
                || value == KEYBOARD_EXTENDED_SET_BYTE) {
                buffer_pos++;
            } else {
                // It's a "simple" key press - print it!
                printf("Pressed S %#02x\n", value);
            }
        } else if (buffer_pos == 1) {
            if (buffer[0] == KEYBOARD_RELEASE_BYTE) {
                // current value is always the "simple" key, but released
                printf("Released S %#02x\n", value);
                buffer_pos = 0;
            } else if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && value == KEYBOARD_RELEASE_BYTE) {
                    buffer_pos++;
            } else if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && value == 0x12) {
                // it's most probably beggining of "print screen pressed"
                // 0xE0 >0x12< 0xE0 0x7C
                buffer_pos++;
            } else {
                // Pressed "extended set" key
                printf("Pressed E %#02x\n", value);
                buffer_pos = 0;
            }
        } else if (buffer_pos == 2) {
            // This is most probably a release of something from extended
            if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && buffer[1] == KEYBOARD_RELEASE_BYTE
                && value == 0x7C) {
                // TODO:
                // it's most probably beggining of "print screen released"
                // 0xE0 0xF0 >0x7C< 0xE0 0xF0 0x12
                buffer_pos++;
            } else if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && buffer[1] == KEYBOARD_RELEASE_BYTE) {
                // Released "extended set" key
                printf("Released E %#02x\n", value);
                buffer_pos = 0;
            } else if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && buffer[1] == 0x12
                && value == KEYBOARD_EXTENDED_SET_BYTE) {
                // continuation of "print screen pressed"
                // 0xE0 0x12 >0xE0< 0x7C
                buffer_pos++;
            }
        } else if (buffer_pos == 3) {
            if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && buffer[1] == KEYBOARD_RELEASE_BYTE
                && buffer[2] == 0x7C
                && value == 0xE0
            ) {
                // continuation of "print screen released"
                // 0xE0 0xF0 0x7C >0xE0< 0xF0 0x12
                buffer_pos++;
            } else if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && buffer[1] == 0x12
                && buffer[2] == KEYBOARD_EXTENDED_SET_BYTE
                && value == 0x7C) {
                // print screen pressed!
                // 0xE0 0x12 0xE0 >0x7C<
                printf("Pressed print screen\n");
                buffer_pos = 0;
            } else {
                buffer_dump_and_panic();
            }
        } else if (buffer_pos == 4) {
            if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && buffer[1] == KEYBOARD_RELEASE_BYTE
                && buffer[2] == 0x7C
                && buffer[3] == 0xE0
                && value == KEYBOARD_RELEASE_BYTE
            ) {
                // continuation of "print screen released"
                // 0xE0 0xF0 0x7C 0xE0 >0xF0< 0x12
                buffer_pos++;
            } else {
                buffer_dump_and_panic();
            }
        }
        else if (buffer_pos == 5) {
            if (buffer[0] == KEYBOARD_EXTENDED_SET_BYTE
                && buffer[1] == KEYBOARD_RELEASE_BYTE
                && buffer[2] == 0x7C
                && buffer[3] == 0xE0
                && buffer[4] == KEYBOARD_RELEASE_BYTE
                && value == 0x12
            ) {
                // print screen released!
                // 0xE0 0xF0 0x7C 0xE0 0xF0 >0x12<
                printf("Released print screen\n");
                buffer_pos = 0;
            } else {
                buffer_dump_and_panic();
            }
        }
    }
    // If no data is ready, this might be a spurious interrupt - just return
}

void keyboard_init()
{
    if (initialized) {
        return;
    }
    initialized = true;

    // First confirm the port
    if (
        (ps2_get_port1_device_type() & PS2_DEVICE_TYPE_MASK)
        == PS2_DEVICE_TYPE_KEYBOARD
    ) {
        puts("PS/2 keyboard at port 1");
    } else if (
        (ps2_get_port2_device_type() & PS2_DEVICE_TYPE_MASK)
        == PS2_DEVICE_TYPE_KEYBOARD
    ) {
        puts("PS/2 keyboard at port 2");
        keyboard_on_port2 = true;
    } else {
        puts("No keyboard detected");
    }

    uint8_t response, value;

    // Set scan code set = 2
    if (!send_command(COM_GET_SET_CURRENT_SCAN_CODE_SET)) {
        puts("Failed to send command for updating scan code set");
        abort();
    }
    if (!send_command(2)) {
        puts("Failed to set scan code set = 2");
        abort();
    }
    // Confirm that scan code has been set to 2
    if (!send_command(COM_GET_SET_CURRENT_SCAN_CODE_SET)) {
        puts("Failed to send command for reading scan code set");
        abort();
    }
    if (!send_command(0)) { // 0 = "get current scan code"
        puts("Failed to request for current scan code");
        abort();
    }
    response = ps2_read_byte_to_result(&value);
    if (response != PS2_RESPONSE_OK) {
        printf("No valid response: %#02x\n", response);
        abort();
    }
    puts("Scan code set = 2");

    uint8_t pic_line = keyboard_on_port2
        ? PIC_LINE_PS2_PORT_2
        : PIC_LINE_PS2_PORT_1;

    // Enable scanning
    if (!send_command(COM_ENABLE_SCANNING)) {
        puts("Failed to send command to enable scanning");
        abort();
    }

    // Enable IRQ
    idt_register_irq_handler(pic_line, &irq_callback);
    pic_enable(pic_line);
    puts("Keyboard initialized");
}
