#include "kernel/drivers/vga/fbcon.h"
#include "kernel/drivers/vga/font.h"

void fbcon_init()
{
    font_load_psf("/initrd/font.psf");
}
