#ifndef KERNEL_DRIVERS_KEYBOARD_H
#define KERNEL_DRIVERS_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

void keyboard_init();

typedef struct {
    uint8_t key_code;
    char ascii; // can be null if not applicable
    bool pressed;
    bool l_shift_pressed;
    bool r_shift_pressed;
    bool l_ctrl_pressed;
    bool r_ctrl_pressed;
    bool l_alt_pressed;
    bool r_alt_pressed;
    bool l_gui_pressed;
    bool r_gui_pressed;
    bool caps_lock_on;
    bool num_lock_on;
    bool scroll_lock_on;
} keyboard_event_t;

// 0b 000 00000
// First three bits: row on a keyboard
// Remaining bits: order on the row
// Using a typical 104-key US QWERTY layout, treating multi-row keys (Enter,
// numpad enter, numpad +) as if they were on the lower row.
#define KCODE_ESC           ((0 << 5) | 0)
#define KCODE_F1            ((0 << 5) | 1)
#define KCODE_F2            ((0 << 5) | 2)
#define KCODE_F3            ((0 << 5) | 3)
#define KCODE_F4            ((0 << 5) | 4)
#define KCODE_F5            ((0 << 5) | 5)
#define KCODE_F6            ((0 << 5) | 6)
#define KCODE_F7            ((0 << 5) | 7)
#define KCODE_F8            ((0 << 5) | 8)
#define KCODE_F9            ((0 << 5) | 9)
#define KCODE_F10           ((0 << 5) | 10)
#define KCODE_F11           ((0 << 5) | 11)
#define KCODE_F12           ((0 << 5) | 12)
#define KCODE_PRNT_SCRN     ((0 << 5) | 13)
#define KCODE_SCROLL_LOCK   ((0 << 5) | 14)

#define KCODE_GRAVE_ACCENT  ((1 << 5) | 0)
#define KCODE_1             ((1 << 5) | 1)
#define KCODE_2             ((1 << 5) | 2)
#define KCODE_3             ((1 << 5) | 3)
#define KCODE_4             ((1 << 5) | 4)
#define KCODE_5             ((1 << 5) | 5)
#define KCODE_6             ((1 << 5) | 6)
#define KCODE_7             ((1 << 5) | 7)
#define KCODE_8             ((1 << 5) | 8)
#define KCODE_9             ((1 << 5) | 9)
#define KCODE_0             ((1 << 5) | 10)
#define KCODE_MINUS         ((1 << 5) | 11)
#define KCODE_EQUALS        ((1 << 5) | 12)
#define KCODE_BACKSLASH     ((1 << 5) | 13)
#define KCODE_BACKSPACE     ((1 << 5) | 14)
#define KCODE_INSERT        ((1 << 5) | 15)
#define KCODE_HOME          ((1 << 5) | 16)
#define KCODE_PAGE_UP       ((1 << 5) | 17)
#define KCODE_NUM_LOCK      ((1 << 5) | 18)
#define KCODE_NUMPAD_SLASH  ((1 << 5) | 19)
#define KCODE_NUMPAD_STAR   ((1 << 5) | 20)
#define KCODE_NUMPAD_MINUS  ((1 << 5) | 21)

#define KCODE_TAB           ((2 << 5) | 0)
#define KCODE_Q             ((2 << 5) | 1)
#define KCODE_W             ((2 << 5) | 2)
#define KCODE_E             ((2 << 5) | 3)
#define KCODE_R             ((2 << 5) | 4)
#define KCODE_T             ((2 << 5) | 5)
#define KCODE_Y             ((2 << 5) | 6)
#define KCODE_U             ((2 << 5) | 7)
#define KCODE_I             ((2 << 5) | 8)
#define KCODE_O             ((2 << 5) | 9)
#define KCODE_P             ((2 << 5) | 10)
#define KCODE_LEFT_BRACKET  ((2 << 5) | 11)
#define KCODE_RIGHT_BRACKET ((2 << 5) | 12)
#define KCODE_DELETE        ((2 << 5) | 13)
#define KCODE_END           ((2 << 5) | 14)
#define KCODE_PAGE_DOWN     ((2 << 5) | 15)
#define KCODE_NUMPAD_7      ((2 << 5) | 16)
#define KCODE_NUMPAD_8      ((2 << 5) | 17)
#define KCODE_NUMPAD_9      ((2 << 5) | 18)

#define KCODE_CAPS_LOCK     ((3 << 5) | 0)
#define KCODE_A             ((3 << 5) | 1)
#define KCODE_S             ((3 << 5) | 2)
#define KCODE_D             ((3 << 5) | 3)
#define KCODE_F             ((3 << 5) | 4)
#define KCODE_G             ((3 << 5) | 5)
#define KCODE_H             ((3 << 5) | 6)
#define KCODE_J             ((3 << 5) | 7)
#define KCODE_K             ((3 << 5) | 8)
#define KCODE_L             ((3 << 5) | 9)
#define KCODE_SEMICOLON     ((3 << 5) | 10)
#define KCODE_APOSTROPHE    ((3 << 5) | 11)
#define KCODE_ENTER         ((3 << 5) | 12)
#define KCODE_NUMPAD_4      ((3 << 5) | 13)
#define KCODE_NUMPAD_5      ((3 << 5) | 14)
#define KCODE_NUMPAD_6      ((3 << 5) | 15)
#define KCODE_NUMPAD_PLUS   ((3 << 5) | 20)

#define KCODE_LEFT_SHIFT    ((4 << 5) | 0)
#define KCODE_Z             ((4 << 5) | 1)
#define KCODE_X             ((4 << 5) | 2)
#define KCODE_C             ((4 << 5) | 3)
#define KCODE_V             ((4 << 5) | 4)
#define KCODE_B             ((4 << 5) | 5)
#define KCODE_N             ((4 << 5) | 6)
#define KCODE_M             ((4 << 5) | 7)
#define KCODE_COMMA         ((4 << 5) | 8)
#define KCODE_FULL_STOP     ((4 << 5) | 9)
#define KCODE_SLASH         ((4 << 5) | 10)
#define KCODE_RIGHT_SHIFT   ((4 << 5) | 11)
#define KCODE_ARROW_UP      ((4 << 5) | 12)
#define KCODE_NUMPAD_1      ((4 << 5) | 13)
#define KCODE_NUMPAD_2      ((4 << 5) | 14)
#define KCODE_NUMPAD_3      ((4 << 5) | 15)

#define KCODE_LEFT_CTRL     ((5 << 5) | 0)
#define KCODE_LEFT_SYSTEM   ((5 << 5) | 1)
#define KCODE_LEFT_ALT      ((5 << 5) | 2)
#define KCODE_SPACE         ((5 << 5) | 3)
#define KCODE_RIGHT_ALT     ((5 << 5) | 4)
#define KCODE_APPS          ((5 << 5) | 5)
#define KCODE_RIGHT_SYSTEM  ((5 << 5) | 6)
#define KCODE_RIGHT_CTRL    ((5 << 5) | 7)
#define KCODE_ARROW_LEFT    ((5 << 5) | 8)
#define KCODE_ARROW_DOWN    ((5 << 5) | 9)
#define KCODE_ARROW_RIGHT   ((5 << 5) | 10)
#define KCODE_NUMPAD_0      ((5 << 5) | 11)
#define KCODE_NUMPAD_DEL    ((5 << 5) | 12)
#define KCODE_NUMPAD_ENTER  ((5 << 5) | 13)

#endif
