#ifndef INCLUDE_KERNEL_TTY_H
#define INCLUDE_KERNEL_TTY_H

#include <stdint.h>
#include <stddef.h>

#define VGA_TAB_WIDTH 8

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

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t) uc | (uint16_t) color << 8;
}

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
