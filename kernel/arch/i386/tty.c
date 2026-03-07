#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "vga.h"
#include <kernel/tty.h>

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

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

void tty_scroll(int line)
{
    int loop;
    char c;
    int startValue = line * (VGA_WIDTH * 2) + VGA_MEMORY;
    for (loop = startValue; loop < VGA_WIDTH * 2; loop++)
    {
        c = *(int*)loop;
        *((int*)loop - (VGA_WIDTH * 2)) = c;
    }
}

void tty_delete_last_line() {
	int x, *ptr;

	for(x = 0; x < VGA_WIDTH * 2; x++) {
		ptr = (int*) 0xB8000 + (VGA_WIDTH * 2) * (VGA_HEIGHT - 1) + x;
		*ptr = 0;
	}
}

void tty_putchar(char c)
{
    int line;
    tty_putentryat(c, terminal_color, terminal_column, terminal_row);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            for (line = 1; line <= VGA_HEIGHT - 1; line++) {
				tty_scroll(line);
			}
			tty_delete_last_line();
			terminal_row = VGA_HEIGHT - 1;
        }
    }
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
