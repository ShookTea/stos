#include <stddef.h>
#include <stdint.h>
#include "kernel/drivers/vga/rgb.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/multiboot2.h"

static uint32_t* addr;
static uint32_t width;
static uint32_t height;
static uint32_t pitch;

void vga_rgb_init()
{
    framebuffer_rgb_config_t* cfg = kmalloc(sizeof(framebuffer_rgb_config_t));
    multiboot2_load_framebuffer_rgb_config(cfg);
    // framebuffer_addr_phys may not be page-aligned; preserve the page offset
    uint32_t page_offset = cfg->framebuffer_addr_phys & 0xFFF;
    addr = (uint32_t*)(FRAMEBUFFER_VIRT_BASE + page_offset);
    width = cfg->framebuffer_width;
    height = cfg->framebuffer_height;
    pitch = cfg->framebuffer_pitch;
    kfree(cfg);

    for (size_t x = 0; x < width; x++) {
        for (size_t y = 0; y < height; y++) {
            vga_rgb_setpixel(x, y, 0xFF000000);
        }
    }
}

size_t vga_rgb_width()
{
    return width;
}

size_t vga_rgb_height()
{
    return height;
}

void vga_rgb_setpixel(size_t x, size_t y, uint32_t argb)
{
    uint32_t* pixel = addr + (y * pitch / 4) + x;
    *pixel = argb;
}
