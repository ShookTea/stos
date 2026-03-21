#include <kernel/terminal.h>
#include <stddef.h>
#include <kernel/serial.h>
#include <kernel/memory/kmalloc.h>
#include "vga.h"

/**
 * Structure describing a single character on screen
 */
typedef struct {
    char codepoint; // displayed character
    uint8_t bg_color;
    uint8_t fg_color;
    uint8_t flags;
} cell_t;

static size_t vga_width;
static size_t vga_height;

// Current cursor location
static size_t cursor_row;
static size_t cursor_column;

// Currently selected color values
static uint8_t bg_color;
static uint8_t fg_color;

static cell_t* cell_buffer;

/**
 * Scroll one line up, adding new line at the bottom
 */
static void terminal_scroll_up()
{
    vga_scroll_up();
    // Move characters up by line
    for (size_t i = 0; i < (vga_height - 1) * vga_width; i++) {
        cell_buffer[i] = cell_buffer[i + vga_width];
    }
    // Clear last line
    for (
        size_t i = (vga_height - 1) * vga_width;
        i < vga_height * vga_width;
        i++
    ) {
        cell_buffer[i].codepoint = '\0';
        cell_buffer[i].bg_color = bg_color;
        cell_buffer[i].fg_color = fg_color;
        cell_buffer[i].flags = 0;
    }
}

static void terminal_scroll_down()
{
    vga_scroll_down();
    // Move characters down by line
    for (size_t i = (vga_height - 1) * vga_width - 1; i >= vga_width; i--) {
        cell_buffer[i] = cell_buffer[i - vga_width];
    }
    // Clear first line
    for (size_t i = 0; i < vga_width; i++) {
        cell_buffer[i].codepoint = '\0';
        cell_buffer[i].bg_color = bg_color;
        cell_buffer[i].fg_color = fg_color;
        cell_buffer[i].flags = 0;
    }
}

void terminal_init()
{
    bg_color = VGA_COLOR_BLACK;
    fg_color = VGA_COLOR_LIGHT_GREY;
    vga_init(vga_entry_color(fg_color, bg_color));
    vga_disable_cursor();
    vga_width = vga_get_columns();
    vga_height = vga_get_rows();

    cell_buffer = kmalloc_flags(
        vga_width * vga_height * sizeof(cell_t),
        KMALLOC_ZERO
    );
}

void terminal_write_char(char c)
{
    serial_put_c(c);

    // TODO: handle special characters and escape codes
    size_t index = cursor_row * vga_width + cursor_column;
    cell_buffer[index].codepoint = c;
    cell_buffer[index].bg_color = bg_color;
    cell_buffer[index].fg_color = fg_color;
    cell_buffer[index].flags = 0;

    vga_putentryat(
        c,
        vga_entry_color(fg_color, bg_color),
        cursor_row,
        cursor_column
    );

    if (++cursor_column == vga_width) {
        cursor_column = 0;
        if (++cursor_row >= vga_height) {
            cursor_row = vga_height - 1;
            terminal_scroll_up();
        }
    }

    vga_set_cursor_position(cursor_row, cursor_column);
}
