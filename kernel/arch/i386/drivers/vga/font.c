#include "kernel/drivers/vga/font.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    /** Bitmap of the glyph */
    uint8_t bitmap[16];
    /** List of all unicode characters supported by this glyph */
    uint32_t* unicode_charaters;
    /** Length of unicode_characters */
    size_t unicode_characters_length;
} glyph_t;

static void parse_psf(uint8_t* file)
{
    if (file[0] != 0x36 || file[1] != 0x04) {
        // Not a PSF1 format - abort
        puts("PSF file is not in a valid PSF1 format");
        return;
    }
    uint8_t mode = file[2];
    bool mode512 = mode & 0x01; // if true, 512 glyphs, otherwise 256 glyphs
    bool unicode_table = (mode & 0x02) || (mode & 0x04);
    uint8_t glyph_size = file[3];
    if (glyph_size != 16) {
        printf("PSF file has glyph_size=%u, only 16 supported\n", glyph_size);
        return;
    }

    if (!mode512) {
        puts("Currently only fonts with mode512 supported");
        return;
    }
    if (!unicode_table) {
        puts("Currently only fonts with unicode table supported");
        return;
    }

    puts("PSF file loaded");
}

void font_load_psf(char* path)
{
    vfs_node_t* node = vfs_resolve(path);
    if (node == NULL) {
        printf("Err loading fonts: path %s doesn't exist\n", path);
        return;
    }
    vfs_file_t* handle = vfs_open(node, VFS_MODE_READONLY);
    uint8_t* file = kmalloc_flags(handle->node->length, KMALLOC_ZERO);
    vfs_read(handle, handle->node->length, file);
    vfs_close(handle);
    parse_psf(file);
    kfree(file);
}

void font_render_char(uint32_t c, size_t x, size_t y, uint32_t fg, uint32_t bg)
{

}
