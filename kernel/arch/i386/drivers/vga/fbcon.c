#include "kernel/drivers/vga/fbcon.h"
#include "kernel/drivers/pit.h"
#include "kernel/drivers/vga/font.h"
#include "kernel/drivers/vga/rgb.h"
#include "kernel/memory/kmalloc.h"
#include <stdint.h>

#define CURSOR_BLINK_INTERVAL_MS 100

uint32_t* char_buffer;
size_t cursor_x = 0;
size_t cursor_y = 0;

static void fbcon_cursor_blink()
{
    // Re-add handler to run this in interval
    pit_register_timeout(CURSOR_BLINK_INTERVAL_MS, fbcon_cursor_blink, NULL);
}

void fbcon_init()
{
    char_buffer = kmalloc(
        sizeof(uint32_t) * fbcon_get_columns() * fbcon_get_rows()
    );
    pit_register_timeout(CURSOR_BLINK_INTERVAL_MS, fbcon_cursor_blink, NULL);
}

void fbcon_putentryat(uint32_t c, uint32_t fg, uint32_t bg, size_t x, size_t y)
{
    size_t index = y * fbcon_get_columns() + x;
    char_buffer[index] = c;
    font_render_char(c, x, y, fg, bg);
}

size_t fbcon_get_columns()
{
    return vga_rgb_width() / PSF1_GLYPH_WIDTH;
}

size_t fbcon_get_rows()
{
    return vga_rgb_height() / PSF1_GLYPH_HEIGHT;
}

void fbcon_scroll_up()
{
    // TODO
}

void fbcon_scroll_down()
{
    // TODO
}

void fbcon_set_cursor_position(size_t x, size_t y)
{
    // TODO
}

void fbcon_enable_cursor()
{
    // TODO
}

void fbcon_disable_cursor()
{
    // TODO
}
