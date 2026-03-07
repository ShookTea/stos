#ifndef INCLUDE_KERNEL_TTY_H
#define INCLUDE_KERNEL_TTY_H

#include <stdint.h>
#include <stddef.h>

/**
 * Clear the content of the terminal and set the default colors
 */
void tty_initialize(void);

/**
 * Sets a color to be used for newly set characters. Use vga_entry_color to
 * generate one from separate background and foreground colors.
 */
void tty_setcolor(uint8_t color);

/**
 * Places selected character on a next location, with tty's color.
 */
void tty_putchar(char c);

/**
 * Prints data of given size, character by character
 */
void tty_write(const char* data, size_t size);

/**
 * Prints a null-terminated string - doesn't require specifying size
 */
void tty_writestring(const char* data);

/**
 * Enables cursor with a given size, with parameters having values from 0 to 15
 */
void tty_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);

/**
 * Disables cursor
 */
void tty_disable_cursor();

/**
 * Places cursor at a specific location on the screen
 */
void tty_update_cursor(size_t x, size_t y);
#endif
