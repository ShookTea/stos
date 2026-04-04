#include "kernel/drivers/vga/fbcon.h"
#include "kernel/drivers/pit.h"
#include "kernel/drivers/vga/font.h"
#include "kernel/drivers/vga/rgb.h"
#include "kernel/memory/kmalloc.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CURSOR_BLINK_INTERVAL_MS 100
#define FBCON_DEFAULT_FG 0xFFFFFFFF
#define FBCON_DEFAULT_BG 0xFF000000
#define CURSOR_HEIGHT 2

uint32_t* char_buffer;
size_t cursor_x = 0;
size_t cursor_y = 0;

static bool cursor_enabled = false;
static bool cursor_blink_state = false;

// Render or erase the cursor underline bar at the current cursor cell.
static void draw_cursor(bool visible)
{
    size_t px = cursor_x * PSF1_GLYPH_WIDTH;
    size_t py = cursor_y * PSF1_GLYPH_HEIGHT;

    if (visible) {
        for (size_t col = 0; col < PSF1_GLYPH_WIDTH; col++) {
            for (size_t row = PSF1_GLYPH_HEIGHT - CURSOR_HEIGHT;
                 row < PSF1_GLYPH_HEIGHT; row++) {
                vga_rgb_setpixel(px + col, py + row, FBCON_DEFAULT_FG);
            }
        }
    } else {
        uint32_t c = char_buffer[cursor_y * fbcon_get_columns() + cursor_x];
        font_render_char(c, px, py, FBCON_DEFAULT_FG, FBCON_DEFAULT_BG);
    }
}

static void fbcon_cursor_blink()
{
    // Re-add handler to run this in interval
    pit_register_timeout(CURSOR_BLINK_INTERVAL_MS, fbcon_cursor_blink, NULL);

    if (cursor_enabled) {
        cursor_blink_state = !cursor_blink_state;
        draw_cursor(cursor_blink_state);
    }
}

void fbcon_init()
{
    size_t buf_size = sizeof(uint32_t) * fbcon_get_columns() * fbcon_get_rows();
    char_buffer = kmalloc(buf_size);
    memset(char_buffer, 0, buf_size);
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
    uint8_t* fb = (uint8_t*)vga_rgb_framebuffer();
    size_t pitch = vga_rgb_pitch();
    size_t h = vga_rgb_height();
    size_t move_px = PSF1_GLYPH_HEIGHT;
    size_t keep = h - move_px;

    memmove(fb, fb + move_px * pitch, keep * pitch);
    memset(fb + keep * pitch, 0, move_px * pitch);

    size_t cols = fbcon_get_columns();
    size_t rows = fbcon_get_rows();
    memmove(
        char_buffer,
        char_buffer + cols,
        (rows - 1) * cols * sizeof(uint32_t)
    );
    memset(char_buffer + (rows - 1) * cols, 0, cols * sizeof(uint32_t));
}

void fbcon_scroll_down()
{
    uint8_t* fb    = (uint8_t*)vga_rgb_framebuffer();
    size_t pitch   = vga_rgb_pitch();
    size_t h       = vga_rgb_height();
    size_t move_px = PSF1_GLYPH_HEIGHT;
    size_t keep    = h - move_px;

    memmove(fb + move_px * pitch, fb, keep * pitch);
    memset(fb, 0, move_px * pitch);

    size_t cols = fbcon_get_columns();
    size_t rows = fbcon_get_rows();
    memmove(char_buffer + cols, char_buffer,
            (rows - 1) * cols * sizeof(uint32_t));
    memset(char_buffer, 0, cols * sizeof(uint32_t));
}

void fbcon_set_cursor_position(size_t x, size_t y)
{
    draw_cursor(false);
    cursor_x = x;
    cursor_y = y;
    if (cursor_enabled) {
        cursor_blink_state = true;
        draw_cursor(true);
    }
}

void fbcon_enable_cursor()
{
    cursor_enabled = true;
    cursor_blink_state = true;
    draw_cursor(true);
}

void fbcon_disable_cursor()
{
    cursor_enabled = false;
    cursor_blink_state = false;
    draw_cursor(false);
}
