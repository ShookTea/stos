#ifndef _KERNEL_DRIVERS_VGA_FONT
#define _KERNEL_DRIVERS_VGA_FONT

#include <stdint.h>
#include <stddef.h>

/**
 * Loads PSF font from given absolute path.
 */
void font_load_psf(char* path);

/**
 * Renders character (given as 4-byte integer for compatibility with Unicode)
 * under given (x,y) position on fbcon, with given ARGB fore- and background
 * colors.
 */
void font_render_char(uint32_t c, size_t x, size_t y, uint32_t fg, uint32_t bg);

#endif
