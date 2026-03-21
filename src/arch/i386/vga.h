#ifndef ARCH_I386_VGA_H
#define ARCH_I386_VGA_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/terminal.h>

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t) uc | (uint16_t) color << 8;
}

/**
 * Initializes VGA and clears the screen
 */
void vga_init(uint8_t color);

/**
 * Sets character at given row and column, with selected color.
 */
void vga_putentryat(char c, uint8_t color, size_t row, size_t column);

/**
 * Returns the total number of columns available
 */
size_t vga_get_columns();

/**
 * Returns the total number of rows available
 */
size_t vga_get_rows();

/**
 * Enables display of the cursor
 */
void vga_enable_cursor();

/**
 * Disables display of the cursor
 */
void vga_disable_cursor();

/**
 * Places cursor at given row and column
 */
void vga_set_cursor_position(size_t row, size_t column);

/**
 * Set default color to be used
 */
void vga_set_color(uint8_t color);

/**
 * Scroll one line up, adding new line at the bottom
 */
void vga_scroll_up();

/**
 * Scroll one line down, adding new line at the top
 */
void vga_scroll_down();

#endif
