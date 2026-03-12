#include "stdlib.h"
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/ps2.h>
#include "../idt/pic.h"
#include "../idt/idt.h"
#include "../io.h"
#include "ps2_defines.h"

#include <stdbool.h>
#include <stdio.h>

#define COM_GET_SET_CURRENT_SCAN_CODE_SET 0xF0
#define COM_ENABLE_SCANNING 0xF4

#define RESPONSE_ACK 0xFA
#define RESPONSE_RESEND 0xFE

static bool keyboard_on_port2 = false;
static bool initialized = false;

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

// This is called when receiving a keyboard interrupt
static void irq_callback()
{

    // In interrupt context, we cannot use timeouts (PIT cannot fire while
    // interrupts are disabled) - we must check if data is available immediately
    // and read it without waiting
    if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER) {
        uint8_t value = inb(PS2_DATA_PORT);
        printf("  keycode: %#02x\n", value);
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
