#include "kernel/spinlock.h"
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

#define KEYBOARD_LISTENERS_COUNT 32

static spinlock_t keyboard_lock = SPINLOCK_INIT;
static bool keyboard_on_port2 = false;
static bool initialized = false;

static struct keyboard_listeners {
    keyboard_callback_t callback;
    bool active;
} listeners[KEYBOARD_LISTENERS_COUNT];

static bool caps_lock_on = false;
static bool num_lock_on = true;
static bool scroll_lock_on = false;
static bool l_shift_pressed = false;
static bool r_shift_pressed = false;
static bool l_ctrl_pressed = false;
static bool r_ctrl_pressed = false;
static bool l_alt_pressed = false;
static bool r_alt_pressed = false;
static bool l_system_pressed = false;
static bool r_system_pressed = false;

static uint8_t buffer[6];
static uint8_t buffer_pos = 0;

int keyboard_register_listener(keyboard_callback_t callback)
{
    spinlock_acquire(&keyboard_lock);
    int id = -1;
    for (int i = 0; i < KEYBOARD_LISTENERS_COUNT; i++) {
        if (!listeners[i].active) {
            id = i;
            break;
        }
    }
    if (id != -1) {
        listeners[id].callback = callback;
        listeners[id].active = true;
    }

    spinlock_release(&keyboard_lock);
    return id;
}

void keyboard_remove_listener(int id)
{
    spinlock_acquire(&keyboard_lock);
    if (id >= 0 && id < KEYBOARD_LISTENERS_COUNT) {
        listeners[id].active = false;
    }
    spinlock_release(&keyboard_lock);
}

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

/**
 * Converts byte+extended+prinstscreen comination to KCODE_ values.
 */
static uint8_t get_key_code(
    uint8_t byte,
    bool extended,
    bool printscreen
) {
    if (printscreen) return KCODE_PRNT_SCRN;
    if (extended) {
        switch (byte) {
            case 0x11: return KCODE_RIGHT_ALT;
            case 0x14: return KCODE_RIGHT_CTRL;
            case 0x1F: return KCODE_LEFT_SYSTEM;
            case 0x27: return KCODE_RIGHT_SYSTEM;
            case 0x2F: return KCODE_APPS;
            case 0x4A: return KCODE_NUMPAD_SLASH;
            case 0x5A: return KCODE_NUMPAD_ENTER;
            case 0x69: return KCODE_END;
            case 0x6B: return KCODE_ARROW_LEFT;
            case 0x6C: return KCODE_HOME;
            case 0x70: return KCODE_INSERT;
            case 0x71: return KCODE_DELETE;
            case 0x72: return KCODE_ARROW_DOWN;
            case 0x74: return KCODE_ARROW_RIGHT;
            case 0x75: return KCODE_ARROW_UP;
            case 0x7A: return KCODE_PAGE_DOWN;
            case 0x7D: return KCODE_PAGE_UP;
            default:
                // TODO: handle it gracefully instead of panicking
                printf("Unrecognized extended key code: %#02x\n", byte);
                abort();
        }
    }
    switch (byte) {
        case 0x01: return KCODE_F9;
        case 0x03: return KCODE_F5;
        case 0x04: return KCODE_F3;
        case 0x05: return KCODE_F1;
        case 0x06: return KCODE_F2;
        case 0x07: return KCODE_F12;
        case 0x09: return KCODE_F10;
        case 0x0A: return KCODE_F8;
        case 0x0B: return KCODE_F6;
        case 0x0C: return KCODE_F4;
        case 0x0D: return KCODE_TAB;
        case 0x0E: return KCODE_GRAVE_ACCENT;

        case 0x11: return KCODE_LEFT_ALT;
        case 0x12: return KCODE_LEFT_SHIFT;
        case 0x14: return KCODE_LEFT_CTRL;
        case 0x15: return KCODE_Q;
        case 0x16: return KCODE_1;
        case 0x1A: return KCODE_Z;
        case 0x1B: return KCODE_S;
        case 0x1C: return KCODE_A;
        case 0x1D: return KCODE_W;
        case 0x1E: return KCODE_2;

        case 0x21: return KCODE_C;
        case 0x22: return KCODE_X;
        case 0x23: return KCODE_D;
        case 0x24: return KCODE_E;
        case 0x25: return KCODE_4;
        case 0x26: return KCODE_3;
        case 0x29: return KCODE_SPACE;
        case 0x2A: return KCODE_V;
        case 0x2B: return KCODE_F;
        case 0x2C: return KCODE_T;
        case 0x2D: return KCODE_R;
        case 0x2E: return KCODE_5;

        case 0x31: return KCODE_N;
        case 0x32: return KCODE_B;
        case 0x33: return KCODE_H;
        case 0x34: return KCODE_G;
        case 0x35: return KCODE_Y;
        case 0x36: return KCODE_6;
        case 0x3A: return KCODE_M;
        case 0x3B: return KCODE_J;
        case 0x3C: return KCODE_U;
        case 0x3D: return KCODE_7;
        case 0x3E: return KCODE_8;

        case 0x41: return KCODE_COMMA;
        case 0x42: return KCODE_K;
        case 0x43: return KCODE_I;
        case 0x44: return KCODE_O;
        case 0x45: return KCODE_0;
        case 0x46: return KCODE_9;
        case 0x49: return KCODE_FULL_STOP;
        case 0x4A: return KCODE_SLASH;
        case 0x4B: return KCODE_L;
        case 0x4C: return KCODE_SEMICOLON;
        case 0x4D: return KCODE_P;
        case 0x4E: return KCODE_MINUS;

        case 0x52: return KCODE_APOSTROPHE;
        case 0x54: return KCODE_LEFT_BRACKET;
        case 0x55: return KCODE_EQUALS;
        case 0x58: return KCODE_CAPS_LOCK;
        case 0x59: return KCODE_RIGHT_SHIFT;
        case 0x5A: return KCODE_ENTER;
        case 0x5B: return KCODE_RIGHT_BRACKET;
        case 0x5D: return KCODE_BACKSLASH;

        case 0x66: return KCODE_BACKSPACE;
        case 0x69: return KCODE_NUMPAD_1;
        case 0x6B: return KCODE_NUMPAD_4;
        case 0x6C: return KCODE_NUMPAD_7;

        case 0x70: return KCODE_NUMPAD_0;
        case 0x71: return KCODE_NUMPAD_DEL;
        case 0x72: return KCODE_NUMPAD_2;
        case 0x73: return KCODE_NUMPAD_5;
        case 0x74: return KCODE_NUMPAD_6;
        case 0x75: return KCODE_NUMPAD_8;
        case 0x76: return KCODE_ESC;
        case 0x77: return KCODE_NUM_LOCK;
        case 0x78: return KCODE_F11;
        case 0x79: return KCODE_NUMPAD_PLUS;
        case 0x7A: return KCODE_NUMPAD_3;
        case 0x7B: return KCODE_NUMPAD_MINUS;
        case 0x7C: return KCODE_NUMPAD_STAR;
        case 0x7D: return KCODE_NUMPAD_9;
        case 0x7E: return KCODE_SCROLL_LOCK;

        case 0x83: return KCODE_F7;
        default:
            // TODO: handle it gracefully instead of panicking
            printf("Unrecognized extended key code: %#02x\n", byte);
            abort();
    }
}

static char get_ascii_code(uint8_t keycode)
{
    bool shift = l_shift_pressed || r_shift_pressed;
    bool upper_case = caps_lock_on ^ shift;

    switch (keycode) {
        case KCODE_1: return shift ? '!' : '1';
        case KCODE_2: return shift ? '@' : '2';
        case KCODE_3: return shift ? '#' : '3';
        case KCODE_4: return shift ? '$' : '4';
        case KCODE_5: return shift ? '%' : '5';
        case KCODE_6: return shift ? '^' : '6';
        case KCODE_7: return shift ? '&' : '7';
        case KCODE_8: return shift ? '*' : '8';
        case KCODE_9: return shift ? '(' : '9';
        case KCODE_0: return shift ? ')' : '0';

        case KCODE_GRAVE_ACCENT: return shift ? '~' : '`';
        case KCODE_MINUS: return shift ? '_' : '-';
        case KCODE_EQUALS: return shift ? '+' : '=';
        case KCODE_BACKSLASH: return shift ? '|' : '\\';
        case KCODE_LEFT_BRACKET: return shift ? '{' : '[';
        case KCODE_RIGHT_BRACKET: return shift ? '}' : ']';
        case KCODE_SEMICOLON: return shift ? ':' : ';';
        case KCODE_APOSTROPHE: return shift ? '"' : '\'';
        case KCODE_COMMA: return shift ? '<' : ',';
        case KCODE_FULL_STOP: return shift ? '>' : '.';
        case KCODE_SLASH: return shift ? '?' : '/';

        case KCODE_Q: return upper_case ? 'Q' : 'q';
        case KCODE_W: return upper_case ? 'W' : 'w';
        case KCODE_E: return upper_case ? 'E' : 'e';
        case KCODE_R: return upper_case ? 'R' : 'r';
        case KCODE_T: return upper_case ? 'T' : 't';
        case KCODE_Y: return upper_case ? 'Y' : 'y';
        case KCODE_U: return upper_case ? 'U' : 'u';
        case KCODE_I: return upper_case ? 'I' : 'i';
        case KCODE_O: return upper_case ? 'O' : 'o';
        case KCODE_P: return upper_case ? 'P' : 'p';
        case KCODE_A: return upper_case ? 'A' : 'a';
        case KCODE_S: return upper_case ? 'S' : 's';
        case KCODE_D: return upper_case ? 'D' : 'd';
        case KCODE_F: return upper_case ? 'F' : 'f';
        case KCODE_G: return upper_case ? 'G' : 'g';
        case KCODE_H: return upper_case ? 'H' : 'h';
        case KCODE_J: return upper_case ? 'J' : 'j';
        case KCODE_K: return upper_case ? 'K' : 'k';
        case KCODE_L: return upper_case ? 'L' : 'l';
        case KCODE_Z: return upper_case ? 'Z' : 'z';
        case KCODE_X: return upper_case ? 'X' : 'x';
        case KCODE_C: return upper_case ? 'C' : 'c';
        case KCODE_V: return upper_case ? 'V' : 'v';
        case KCODE_B: return upper_case ? 'B' : 'b';
        case KCODE_N: return upper_case ? 'N' : 'n';
        case KCODE_M: return upper_case ? 'M' : 'm';

        case KCODE_NUMPAD_STAR: return '*';
        case KCODE_NUMPAD_MINUS: return '-';
        case KCODE_NUMPAD_PLUS: return '+';
        case KCODE_NUMPAD_SLASH: return '/';
        // TODO: instead of checking it here, we should probably return
        // different KCODE_ when num lock is off
        case KCODE_NUMPAD_0: return num_lock_on ? '0' : 0;
        case KCODE_NUMPAD_1: return num_lock_on ? '1' : 0;
        case KCODE_NUMPAD_2: return num_lock_on ? '2' : 0;
        case KCODE_NUMPAD_3: return num_lock_on ? '3' : 0;
        case KCODE_NUMPAD_4: return num_lock_on ? '4' : 0;
        case KCODE_NUMPAD_5: return num_lock_on ? '5' : 0;
        case KCODE_NUMPAD_6: return num_lock_on ? '6' : 0;
        case KCODE_NUMPAD_7: return num_lock_on ? '7' : 0;
        case KCODE_NUMPAD_8: return num_lock_on ? '8' : 0;
        case KCODE_NUMPAD_9: return num_lock_on ? '9' : 0;
        case KCODE_NUMPAD_DEL: return num_lock_on ? ',' : 0x7F;

        case KCODE_BACKSPACE: return '\b';
        case KCODE_TAB: return '\t';
        case KCODE_DELETE: return 0x7F;
        case KCODE_ENTER: return '\n';
        case KCODE_NUMPAD_ENTER: return '\n';
        case KCODE_SPACE: return ' ';
    }
    return 0;
}

static keyboard_event_t build_keyboard_event(uint8_t keycode, bool release)
{
    keyboard_event_t event = {
        .key_code = keycode,
        .ascii = get_ascii_code(keycode),
        .pressed = !release,
        .l_shift_pressed = l_shift_pressed,
        .r_shift_pressed = r_shift_pressed,
        .l_ctrl_pressed = l_ctrl_pressed,
        .r_ctrl_pressed = r_ctrl_pressed,
        .l_alt_pressed = l_alt_pressed,
        .r_alt_pressed = r_alt_pressed,
        .l_system_pressed = l_system_pressed,
        .r_system_pressed = r_system_pressed,
        .caps_lock_on = caps_lock_on,
        .num_lock_on = num_lock_on,
        .scroll_lock_on = scroll_lock_on,
    };
    return event;
}

static void key_handler(
    uint8_t byte,
    bool release,
    bool extended,
    bool printscreen
) {
    uint8_t keycode = get_key_code(byte, extended, printscreen);
    if (!release && keycode == KCODE_CAPS_LOCK) {
        caps_lock_on = !caps_lock_on;
    } else if (!release && keycode == KCODE_NUM_LOCK) {
        num_lock_on = !num_lock_on;
    } else if (!release && keycode == KCODE_SCROLL_LOCK) {
        scroll_lock_on = !scroll_lock_on;
    } else if (keycode == KCODE_LEFT_SHIFT) {
        l_shift_pressed = !release;
    } else if (keycode == KCODE_RIGHT_SHIFT) {
        r_shift_pressed = !release;
    } else if (keycode == KCODE_LEFT_ALT) {
        l_alt_pressed = !release;
    } else if (keycode == KCODE_RIGHT_ALT) {
        r_alt_pressed = !release;
    } else if (keycode == KCODE_LEFT_CTRL) {
        l_ctrl_pressed = !release;
    } else if (keycode == KCODE_RIGHT_CTRL) {
        r_ctrl_pressed = !release;
    } else if (keycode == KCODE_LEFT_SYSTEM) {
        l_system_pressed = !release;
    } else if (keycode == KCODE_RIGHT_SYSTEM) {
        r_system_pressed = !release;
    }
    keyboard_event_t evt = build_keyboard_event(keycode, release);

    for (int i = 0; i < KEYBOARD_LISTENERS_COUNT; i++) {
        if (listeners[i].active) {
            (listeners[i].callback)(evt);
        }
    }
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
                key_handler(value, false, false, false);
                buffer_pos = 0;
            }
        } else if (buffer_pos == 1) {
            if (buffer[0] == KEYBOARD_RELEASE_BYTE) {
                // current value is always the "simple" key, but released
                key_handler(value, true, false, false);
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
                key_handler(value, false, true, false);
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
                key_handler(value, true, true, false);
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
                key_handler(0, false, false, true);
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
                key_handler(0, true, false, true);
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
