#ifndef _KERNEL_DRIVERS_VGA_FBCON
#define _KERNEL_DRIVERS_VGA_FBCON

#include <stddef.h>
#include <stdint.h>

/**
 * Initializes "framebuffer console" - a textmode-like layer on top of RGB mode.
 */
void fbcon_init();

/**
 * Sets character at given row and column, with selected color.
 */
void fbcon_putentryat(uint32_t c, uint32_t fg, uint32_t bg, size_t x, size_t y);

/**
 * Returns the total number of columns available
 */
size_t fbcon_get_columns();

/**
 * Returns the total number of rows available
 */
size_t fbcon_get_rows();

#endif
