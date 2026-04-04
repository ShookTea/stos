#include "kernel/drivers/vga/text_mode.h"
#include "../../io.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t) uc | (uint16_t) color << 8;
}

static uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;
static uint8_t terminal_color;

void vga_text_init(uint8_t color)
{
    terminal_color = color;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void vga_text_putentryat(char c, uint8_t color, size_t row, size_t column)
{
    const size_t index = row * VGA_WIDTH + column;
    terminal_buffer[index] = vga_entry(c, color);
}

size_t vga_text_get_columns()
{
    return VGA_WIDTH;
}

size_t vga_text_get_rows()
{
    return VGA_HEIGHT;
}

void vga_text_enable_cursor()
{
    outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void vga_text_disable_cursor()
{
    outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void vga_text_set_cursor_position(size_t row, size_t column)
{
    uint16_t pos = row * VGA_WIDTH + column;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void vga_text_scroll_up()
{
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

void vga_text_scroll_down()
{
    // Shift lines down using uint16_t entries
    for (size_t i = VGA_HEIGHT * VGA_WIDTH - 1; i >= VGA_WIDTH; i--) {
        terminal_buffer[i] = terminal_buffer[i - VGA_WIDTH];
    }

    // Clear first line
    uint16_t blank = vga_entry(' ', terminal_color);
    for (size_t i = 0; i < VGA_WIDTH; i++) {
        terminal_buffer[i] = blank;
    }
}
