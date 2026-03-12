#ifndef ARCH_I386_DRIVERS_KEYBOARD_DEFINES
#define ARCH_I386_DRIVERS_KEYBOARD_DEFINES

/**
 * Keyboard scan code set 2 is nice enough in that it's (slightly) predictable:
 * - A lot of keys (including all printable ones) have one byte for "pressed"
 *   state (0x1C for "A"), while for release they first send 0xF0, then its
 *   own byte again (so 0xF0 0x1C for "A")
 * - some special characters first send 0xE0 and then their own byte when
 *   pressed; when released, they send 0xE0 0xF0 and their byte again (for
 *   example, of Right Ctrl: 0xE0 0x14 for pressing, 0xE0 0xF0 0x14 for release)
 * - neither 0xF0 nor 0xE0 are representing any other key
 *
 * but there are few other crazy combinations:
 * - print screen:
 *   press: 0xE0 0x12 0xE0 0x7C
 *   release: 0xE0 0xF0 0x7C 0xE0 0xF0 0x12
 */

#define KEYBOARD_EXTENDED_SET_BYTE 0xE0
#define KEYBOARD_RELEASE_BYTE 0xF0

#endif
