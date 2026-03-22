#ifndef INCLUDE_KERNEL_TERMINAL_H
#define INCLUDE_KERNEL_TERMINAL_H

#include <stddef.h>

/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

/**
 * Initialize terminal
 */
void terminal_init();

/**
 * Write a single character to the terminal and moves the cursor. Special cases:
 * - If the character is \n, goes to the next line immediataly.
 * - If the character is \t, prints multiple spaces to align to 8 columns
 * - If the character is \b, removes the previous character
 *   - it properly handles situations where the previous character was \t,
 *     removing all spaces printed by it.
 * - If the character is \033, it initializes the escape sequence. All following
 *   characters won't be printed until a valid end of the sequence is received,
 *   or if the sequence can't be understood by the terminal.
 *
 * === Escape sequences ===
 * Currently the only interpreted sequence is the Control Sequence Introducer
 * commands, which start with "\033[", then:
 * - it can optionally start with a question mark ('?')
 * - then it is followed by a set of zero, one, or more numerical arguments
 *   separated by semicolons
 * - and ends with a mandatory single letter that determines the command.
 *
 * Special rules about numerical arguments:
 * - There is always at least one argument. If the sequence doesn't receive
 *   any arguments, it is interpreted as if it received one argument with value
 *   set to zero. For example, "\033[K" is equivalent to "\033[0K"
 * - If entry separated by semicolon is empty, it is also interpreted as zero.
 *   For example, arguments "1;;3" are equivalent to "1;0;3"
 * - Some commands treat 0 as 1 in order to make missing parameters useful.
 *
 * Supported CSI commands are:
 * - A - move cursor arg[0] cells up (defaults to 1)
 * - B - move cursor arg[0] cells down (defaults to 1)
 * - C - move cursor arg[0] cells forward (defaults to 1)
 * - D - move cursor arg[0] cells back (defaults to 1)
 * - E - move cursor to the beginning of the line, arg[0] rows down
 *       (defaults to 1)
 * - F - move cursor to the beginning of the line, arg[0] rows up
 *       (defaults to 1)
 * - G - move cursor to column arg[0] (1-based indexing, defaults to 1)
 * - H - move cursor to row arg[0], column arg[1]
 *       (1-based indexing, defaults to 1)
 * - J - erase in display (cursor position won't change)
 *     - if args[0] = 0 (default), clear from cursor to end of the screen
 *     - if args[0] = 1, clear from cursor to beginning of the screen
 *     - if args[0] = 2, clear entire screen
 * - K - erase in line (cursor position won't change)
 *     - if args[0] = 0 (default), clear from cursor to end of the line
 *     - if args[0] = 1, clear from cursor to beginning of the line
 *     - if args[0] = 2, clear entire line
 * - S - scroll up by arg[0] (defaults to 1) lines, adding new lines at the
 *       bottom of the screen
 * - T - scroll down by arg[0] (defaults to 1) lines, adding new lines at the
 *       top of the screen
 */
void terminal_write_char(char c);

/**
 * Write data with a given size to the terminal
 */
inline void terminal_write(const char* data, size_t size)
{
   	for (size_t i = 0; i < size; i++) {
        terminal_write_char(data[i]);
    }
}

/**
 * Write a null-terminated string to the terminal
 */
inline void terminal_write_string(char* s)
{
    size_t len = 0;
    while (s[len]) {
        terminal_write_char(s[len]);
        len++;
    }
}

// TODO: remove functions below, to be replaced with correct escape codes
void terminal_enable_cursor();
void terminal_disable_cursor();
void terminal_set_bg_color(enum vga_color color);
void terminal_set_fg_color(enum vga_color color);

#endif
