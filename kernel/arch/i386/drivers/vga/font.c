#include "kernel/drivers/vga/font.h"
#include "kernel/drivers/vga/rgb.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// Glyph bitmaps copied out of the font file. Each row is one byte; bit 7 is
// the leftmost pixel.
static uint8_t font_glyphs[PSF1_GLYPH_COUNT][PSF1_GLYPH_HEIGHT];

// Sorted (codepoint → glyph index) table for O(log n) lookup.
typedef struct {
    uint32_t codepoint;
    uint16_t glyph_index;
} unicode_entry_t;

static unicode_entry_t* unicode_map = NULL;
static size_t unicode_map_size = 0;
static bool font_loaded = false;

/**
 * Parsing unicode table from the PSF file
 */
static void parse_unicode_table(uint16_t* table, size_t u16_count)
{
    // First pass: count mappable entries so we can allocate exactly.
    size_t count = 0;
    uint16_t glyphs_done = 0;
    for (size_t i = 0; i < u16_count && glyphs_done < PSF1_GLYPH_COUNT; i++) {
        uint16_t v = table[i];
        if (v == 0xFFFF) {
            glyphs_done++;
        } else if (v != 0xFFFE) {
            if (v >= 0xD800 && v <= 0xDBFF) i++; // consume low surrogate
            count++;
        }
    }

    unicode_map = kmalloc(count * sizeof(unicode_entry_t));
    if (!unicode_map) {
        printf(
            "font: failed to allocate unicode map (%u entries)\n",
            (uint32_t)count
        );
        return;
    }

    // Second pass: fill the map
    size_t idx = 0;
    uint16_t glyph_index = 0;
    for (size_t i = 0; i < u16_count && glyph_index < PSF1_GLYPH_COUNT; i++) {
        uint16_t v = table[i];
        if (v == 0xFFFF) {
            glyph_index++;
        } else if (v == 0xFFFE) {
            // segment separator – keep current glyph_index, continue
        } else {
            uint32_t cp;
            if (v >= 0xD800 && v <= 0xDBFF && i + 1 < u16_count) {
                uint16_t low = table[++i];
                cp = 0x10000u + ((uint32_t)(v - 0xD800) << 10) + (low - 0xDC00);
            } else {
                cp = v;
            }
            unicode_map[idx].codepoint   = cp;
            unicode_map[idx].glyph_index = glyph_index;
            idx++;
        }
    }
    unicode_map_size = idx;

    // Sort by codepoint (insertion sort – adequate for typical font sizes).
    for (size_t i = 1; i < unicode_map_size; i++) {
        unicode_entry_t key = unicode_map[i];
        size_t j = i;
        while (j > 0 && unicode_map[j - 1].codepoint > key.codepoint) {
            unicode_map[j] = unicode_map[j - 1];
            j--;
        }
        unicode_map[j] = key;
    }

    printf("font: %u unicode mappings loaded\n", (uint32_t)unicode_map_size);
}

/**
 * PSF1 parser
 */
static void parse_psf(uint8_t* file, size_t file_size)
{
    if (file[0] != 0x36 || file[1] != 0x04) {
        puts("font: not a valid PSF1 file");
        return;
    }
    uint8_t mode       = file[2];
    uint8_t glyph_size = file[3];
    bool mode512       = mode & 0x01;
    bool has_unicode   = (mode & 0x02) || (mode & 0x04);

    if (glyph_size != PSF1_GLYPH_HEIGHT) {
        printf(
            "font: glyph_size=%u, only %u supported\n",
            glyph_size,
            PSF1_GLYPH_HEIGHT
        );
        return;
    }
    if (!mode512) {
        puts("font: only 512-glyph PSF1 fonts supported");
        return;
    }
    if (!has_unicode) {
        puts("font: only PSF1 fonts with unicode table supported");
        return;
    }

    // Copy glyph bitmaps.
    uint8_t* glyph_data = file + 4;
    for (int i = 0; i < PSF1_GLYPH_COUNT; i++)
        memcpy(
            font_glyphs[i],
            glyph_data + i * PSF1_GLYPH_HEIGHT,
            PSF1_GLYPH_HEIGHT
        );

    // Parse unicode table (starts immediately after glyph data).
    size_t unicode_offset = 4 + (size_t)PSF1_GLYPH_COUNT * PSF1_GLYPH_HEIGHT;
    if (unicode_offset < file_size) {
        uint16_t* table = (uint16_t*)(file + unicode_offset);
        size_t u16_count = (file_size - unicode_offset) / 2;
        parse_unicode_table(table, u16_count);
    }

    font_loaded = true;
    printf(
        "font: PSF1 font loaded (%u glyphs, %ux%u)\n",
        PSF1_GLYPH_COUNT,
        PSF1_GLYPH_WIDTH,
        PSF1_GLYPH_HEIGHT
    );
}

void font_load_psf(char* path)
{
    vfs_node_t* node = vfs_resolve(path);
    if (node == NULL) {
        printf("font: path %s not found\n", path);
        return;
    }
    vfs_file_t* handle = vfs_open(node, VFS_MODE_READONLY);
    size_t size = handle->node->length;
    uint8_t* file = kmalloc_flags(size, KMALLOC_ZERO);
    vfs_read(handle, size, file);
    vfs_close(handle);
    parse_psf(file, size);
    kfree(file);
}

static int32_t find_glyph(uint32_t codepoint)
{
    size_t lo = 0, hi = unicode_map_size;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (unicode_map[mid].codepoint == codepoint) {
            return unicode_map[mid].glyph_index;
        }
        else if (unicode_map[mid].codepoint  < codepoint) {
            lo = mid + 1;
        }
        else {
            hi = mid;
        }
    }
    return -1;
}

void font_render_char(uint32_t c, size_t x, size_t y, uint32_t fg, uint32_t bg)
{
    if (!font_loaded) {
        return;
    }

    int32_t idx = find_glyph(c);
    if (idx < 0) idx = find_glyph(0xFFFD); // Unicode replacement character
    if (idx < 0) idx = 0; // last resort: glyph 0

    const uint8_t* bitmap = font_glyphs[idx];
    for (size_t row = 0; row < PSF1_GLYPH_HEIGHT; row++) {
        uint8_t bits = bitmap[row];
        for (size_t col = 0; col < PSF1_GLYPH_WIDTH; col++) {
            uint32_t color = (bits & (0x80u >> col)) ? fg : bg;
            vga_rgb_setpixel(x + col, y + row, color);
        }
    }
}
