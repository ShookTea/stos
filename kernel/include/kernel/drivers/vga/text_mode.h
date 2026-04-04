#ifndef _KERNEL_DRIVERS_VGA_TEXT_H
#define _KERNEL_DRIVERS_VGA_TEXT_H

#include <stdint.h>
#include <stddef.h>

/**
 * Initializes VGA and clears the screen
 */
void vga_text_init(uint8_t color);

/**
 * Sets character at given row and column, with selected color.
 */
void vga_text_putentryat(char c, uint8_t color, size_t row, size_t column);

/**
 * Returns the total number of columns available
 */
size_t vga_text_get_columns();

/**
 * Returns the total number of rows available
 */
size_t vga_text_get_rows();

/**
 * Enables display of the cursor
 */
void vga_text_enable_cursor();

/**
 * Disables display of the cursor
 */
void vga_text_disable_cursor();

/**
 * Places cursor at given row and column
 */
void vga_text_set_cursor_position(size_t row, size_t column);

/**
 * Scroll one line up, adding new line at the bottom
 */
void vga_text_scroll_up();

/**
 * Scroll one line down, adding new line at the top
 */
void vga_text_scroll_down();

#endif
