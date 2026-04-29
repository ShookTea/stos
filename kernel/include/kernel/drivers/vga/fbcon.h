#ifndef _KERNEL_DRIVERS_VGA_FBCON
#define _KERNEL_DRIVERS_VGA_FBCON

#include "kernel/drivers/vga/font.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    font_mode_t font_mode;
    uint32_t codepoint;
    uint8_t fg; // enum vga_color index (0-15)
    uint8_t bg; // enum vga_color index (0-15)
    uint8_t _pad[2];
} fbcon_cell_t;

/**
 * Initializes "framebuffer console" - a textmode-like layer on top of RGB mode.
 */
void fbcon_init();

/**
 * Sets character at given row and column, with selected color.
 */
void fbcon_putentryat(
    font_mode_t font_mode,
    uint32_t c,
    uint8_t fg, uint8_t bg,
    size_t x, size_t y
);

/**
 * Returns the total number of columns available
 */
size_t fbcon_get_columns();

/**
 * Returns the total number of rows available
 */
size_t fbcon_get_rows();

void fbcon_scroll_up();
void fbcon_scroll_down();
void fbcon_set_cursor_position(size_t x, size_t y);
void fbcon_enable_cursor();
void fbcon_disable_cursor();

#endif
