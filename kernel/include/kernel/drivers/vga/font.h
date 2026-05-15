#ifndef _KERNEL_DRIVERS_VGA_FONT
#define _KERNEL_DRIVERS_VGA_FONT

#include <stdint.h>
#include <stddef.h>

#define PSF1_GLYPH_COUNT  512
#define PSF1_GLYPH_HEIGHT 16
#define PSF1_GLYPH_WIDTH  8

typedef enum {
    FONT_MODE_NORMAL = 0,
    FONT_MODE_BOLD,
} font_mode_t;

#define FONT_DECORATION_UNDERLINE 1
#define FONT_DECORATION_OVERLINE 2
#define FONT_DECORATION_STRIKE 4
#define FONT_DECORATION_SLOW_BLINK 8
#define FONT_DECORATION_QUICK_BLINK 16

/**
 * Loads PSF font from given absolute path.
 */
void font_load_psf(font_mode_t font_mode, char* path);

/**
 * Renders character (given as 4-byte integer for compatibility with Unicode)
 * under given (x,y) position on fbcon, with given ARGB fore- and background
 * colors.
 */
void font_render_char(
    font_mode_t font_mode,
    uint8_t font_decor,
    uint32_t c,
    size_t x,
    size_t y,
    uint32_t fg,
    uint32_t bg
);

#endif
