#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "io.h"

#include <kernel/tty.h>
#include <kernel/serial.h>

#define VGA_WIDTH     80
#define VGA_HEIGHT    25
#define VGA_MEMORY    0xB8000

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

void tty_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void tty_setcolor(uint8_t color)
{
    terminal_color = color;
}

void tty_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void tty_scroll() {
    // Shift lines up using uint16_t entries
    for (size_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        terminal_buffer[i] = terminal_buffer[i + VGA_WIDTH];
    }

    // Clear last line
    uint16_t blank = vga_entry(' ', terminal_color);
    for (size_t i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        terminal_buffer[i] = blank;
    }
}

void tty_putchar(char c) {
    serial_put_c(c);
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row >= VGA_HEIGHT) {
            tty_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
        tty_update_cursor(terminal_column, terminal_row);
        return;
    }

    if (c == '\t') {
        size_t new_terminal_column =
            ((terminal_column + VGA_TAB_WIDTH) / VGA_TAB_WIDTH)
            * VGA_TAB_WIDTH;
        if (new_terminal_column > VGA_WIDTH) {
            new_terminal_column = VGA_WIDTH;
        }
        for (size_t i = terminal_column; i < new_terminal_column; i++) {
            tty_putchar(' ');
        }
        return;
    }

    // Backspace received
    if (c == '\b') {
        if (terminal_column == 0) {
            terminal_column = VGA_WIDTH - 1;
            terminal_row--;
        } else {
            terminal_column--;
        }
    } else {
        tty_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row >= VGA_HEIGHT) {
                tty_scroll();
                terminal_row = VGA_HEIGHT - 1;
            }
        }
    }
    tty_update_cursor(terminal_column, terminal_row);
}

void tty_write(const char* data, size_t size)
{
   	for (size_t i = 0; i < size; i++) {
        tty_putchar(data[i]);
    }
}

void tty_writestring(const char *data)
{
    tty_write(data, strlen(data));
}

void tty_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
   	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void tty_disable_cursor()
{
    outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void tty_update_cursor(size_t x, size_t y)
{
    uint16_t pos = y * VGA_WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
