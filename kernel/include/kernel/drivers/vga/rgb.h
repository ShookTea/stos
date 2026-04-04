#ifndef _KERNEL_DRIVERS_VGA_RGB_H
#define _KERNEL_DRIVERS_VGA_RGB_H

#include <stddef.h>
#include <stdint.h>

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

#endif
