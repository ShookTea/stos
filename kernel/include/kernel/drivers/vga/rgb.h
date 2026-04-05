#ifndef _KERNEL_DRIVERS_VGA_RGB_H
#define _KERNEL_DRIVERS_VGA_RGB_H

#include <stddef.h>
#include <stdint.h>
#include <kernel/terminal.h>

static inline uint32_t vga_color_to_argb(enum vga_color color)
{
    switch (color) {
        case VGA_COLOR_BLACK: return 0xFF000000;
        case VGA_COLOR_BLUE: return 0xFF001290;
        case VGA_COLOR_GREEN: return 0xFF008F15;
        case VGA_COLOR_CYAN: return 0xFF009092;
        case VGA_COLOR_RED: return 0xFF9B1708;
        case VGA_COLOR_MAGENTA: return 0xFF9A2091;
        case VGA_COLOR_BROWN: return 0xFF949119;
        case VGA_COLOR_LIGHT_GREY: return 0xFFB8B8B8;
        case VGA_COLOR_DARK_GREY: return 0xFF686868;
        case VGA_COLOR_LIGHT_BLUE: return 0xFF0027FB;
        case VGA_COLOR_LIGHT_GREEN: return 0xFF00F92C;
        case VGA_COLOR_LIGHT_CYAN: return 0xFF00FCFE;
        case VGA_COLOR_LIGHT_RED: return 0xFFFF3016;
        case VGA_COLOR_LIGHT_MAGENTA: return 0xFFFF3FFC;
        case VGA_COLOR_LIGHT_BROWN: return 0xFFFFFD33;
        case VGA_COLOR_WHITE: return 0xFFFFFFFF;
        default: return 0xFF000000;
    }
}

/**
 * Initializes VGA in RGB mode and clears the screen
 */
void vga_rgb_init();

/**
 * Returns width of the screen in pixels
 */
size_t vga_rgb_width();

/**
 * Returns height of the screen in pixels
 */
size_t vga_rgb_height();

/**
 * Set pixel color on given coordinates
 */
void vga_rgb_setpixel(size_t x, size_t y, uint32_t argb);

/**
 * Returns pitch (bytes per pixel row) of the framebuffer
 */
size_t vga_rgb_pitch();

/**
 * Returns pointer to the start of the framebuffer
 */
uint32_t* vga_rgb_framebuffer();

#endif
