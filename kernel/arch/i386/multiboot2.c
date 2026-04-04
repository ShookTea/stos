#include "kernel/paging.h"
#include <kernel/memory/pmm.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <kernel/multiboot2.h>
#include <kernel/acpi.h>

static multiboot_info_t* mbi;
static char boot_command_line[64];
static char boot_loader_name[64];
static uint32_t load_base_addr = 0;

// Memory map storage
#define MAX_MMAP_ENTRIES 16

static saved_mmap_entry_t saved_mmap[MAX_MMAP_ENTRIES];
static uint32_t saved_mmap_count = 0;
static uint32_t pmm_max_memory = 0;  // Maximum usable memory address

#define MAX_BOOT_MODULE_ENTRIES 16
static multiboot_tag_boot_module_t boot_modules[MAX_BOOT_MODULE_ENTRIES];
// Set max boot module name to 64 characters - should be more than enough
static char boot_module_names[MAX_BOOT_MODULE_ENTRIES][64];
static uint8_t saved_boot_modules_count = 0;

static bool rgb_mode = false;
static uint32_t framebuffer_addr_phys = 0;
static uint32_t framebuffer_width = 0;
static uint32_t framebuffer_height = 0;
static uint32_t framebuffer_pitch = 0;

static void multiboot2_process_memory_map()
{
    if (saved_mmap_count == 0) {
        printf("WARNING: No memory map entries found\n");
        return;
    }

    printf("Memory map entries: %u\n", saved_mmap_count);

    // Find the maximum usable memory address
    pmm_max_memory = 0;
    for (uint32_t i = 0; i < saved_mmap_count; i++) {
        saved_mmap_entry_t* entry = &saved_mmap[i];

        // Only consider available RAM (type 1)
        if (entry->type == 1) {
            // Calculate end address (base + length)
            uint64_t base = ((uint64_t)entry->base_high << 32) |
                           entry->base_low;
            uint64_t length = ((uint64_t)entry->length_high << 32) |
                             entry->length_low;
            uint64_t end = base + length;

            // We only support 32-bit addressing for now
            if (end > 0xFFFFFFFF) {
                end = 0xFFFFFFFF;
            }

            if ((uint32_t)end > pmm_max_memory) {
                pmm_max_memory = (uint32_t)end;
            }
        }
    }

    printf("Maximum memory address: %#x (%u MB)\n",
           pmm_max_memory, pmm_max_memory / (1024 * 1024));
}

void multiboot2_init(multiboot_info_t* multiboot_info)
{
    mbi = multiboot_info;
    uint64_t mb2_end = mbi->total_size + (uint32_t)mbi;
    // adding 8 bytes here to adjust for the multiboot_info_t header
    multiboot_tag_t* tag = (multiboot_tag_t*)((uint8_t*)mbi + 8);

    while ((uint32_t)tag < mb2_end) {
        switch (tag->type) {
            case MULTIBOOT2_TAG_TYPE_END:
                // We've reached the END tag, process memory map
                multiboot2_process_memory_map();
                return;
            case MULTIBOOT2_TAG_TYPE_BOOT_COMMAND_LINE: {
                multiboot_tag_boot_command_line_t* cl =
                    (multiboot_tag_boot_command_line_t*)tag;
                size_t len = strlen(cl->command_line);
                if (len > 64) len = 64;
                memset(boot_command_line, 0, 64);
                memcpy(boot_command_line, cl->command_line, len);
                boot_command_line[63] = 0;
                break;
            }
            case MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME: {
                multiboot_tag_boot_loader_name_t* bln =
                    (multiboot_tag_boot_loader_name_t*)tag;
                size_t len = strlen(bln->name);
                if (len > 64) len = 64;
                memset(boot_loader_name, 0, 64);
                memcpy(boot_loader_name, bln->name, len);
                boot_loader_name[63] = 0;
                break;
            }
            case MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR:
                load_base_addr = ((multiboot_tag_load_base_addr_t*)tag)
                    ->load_base_addr;
                break;
            case MULTIBOOT2_TAG_TYPE_MEMORY_MAP: {
                multiboot_tag_memory_map_t* mmap =
                    (multiboot_tag_memory_map_t*)tag;
                uint32_t entries_count = (mmap->size - 16) / mmap->entry_size;

                if (entries_count > MAX_MMAP_ENTRIES) {
                    printf("WARNING: Too many memory map entries, truncating to %d\n",
                           MAX_MMAP_ENTRIES);
                    entries_count = MAX_MMAP_ENTRIES;
                }

                // Save memory map entries
                saved_mmap_count = entries_count;
                for (uint32_t i = 0; i < entries_count; i++) {
                    multiboot_tag_memory_map_entry_t* entry = &mmap->entries[i];
                    saved_mmap[i].base_low = entry->base_addr_low;
                    saved_mmap[i].base_high = entry->base_addr_high;
                    saved_mmap[i].length_low = entry->length_low;
                    saved_mmap[i].length_high = entry->length_high;
                    saved_mmap[i].type = entry->type;
                }
                break;
            }
            case MULTIBOOT2_TAG_TYPE_ACPI_OLD: {
                multiboot_tag_boot_acpi_old_t* acpi =
                    (multiboot_tag_boot_acpi_old_t*)tag;
                acpi_init_old(acpi->rsdp);
                break;
            }
            case MULTIBOOT2_TAG_TYPE_ACPI_NEW: {
                multiboot_tag_boot_acpi_new_t* acpi =
                    (multiboot_tag_boot_acpi_new_t*)tag;
                acpi_init_new(acpi->rsdp);
                break;
            }
            case MULTIBOOT2_TAG_TYPE_MODULES: {
                multiboot_tag_boot_module_t* module =
                    (multiboot_tag_boot_module_t*)tag;
                if (saved_boot_modules_count < MAX_BOOT_MODULE_ENTRIES) {
                    boot_modules[saved_boot_modules_count] = *module;
                    strncpy(
                        boot_module_names[saved_boot_modules_count],
                        module->module_name,
                        64
                    );
                    // Clear last byte
                    boot_module_names[saved_boot_modules_count][63] = '\0';
                    saved_boot_modules_count++;
                }

                break;
            }
            case MULTIBOOT2_TAG_TYPE_FRAMEBUFFER_INFO: {
                multiboot_tag_framebuffer_info_t* fb_inf =
                    (multiboot_tag_framebuffer_info_t*)tag;
                printf(
                    "  framebuffer size: %u x %u, color mode %u, addr: %x %x\n",
                    fb_inf->width,
                    fb_inf->height,
                    fb_inf->color_type,
                    fb_inf->framebuffer_addr_high,
                    fb_inf->framebuffer_addr_low
                );
                // uint32_t* fb = (uint32_t*)fb_inf->framebuffer_addr_low;
                // for (size_t x = 0; x < fb_inf->width; x++) {
                //     for (size_t y = 0; y < fb_inf->height; y++) {
                //         uint32_t* pixel = fb + y * (fb_inf->pitch / 4) + x;
                //         if ((x % 32 == 0) || (y % 32 == 0)) {
                //             *pixel = 0xFFFF0000;  // Red grid lines (ARGB)
                //         } else {
                //             *pixel = 0xFF0000FF;  // Blue background
                //         }
                //     }
                // }
                if (fb_inf->color_type == 1) {
                    if (fb_inf->framebuffer_addr_high != 0) {
                        printf("WARNING: Framebuffer above 4GB, not supported in 32-bit mode\n");
                    } else {
                        rgb_mode = true;
                        framebuffer_addr_phys = fb_inf->framebuffer_addr_low;
                        framebuffer_height = fb_inf->height;
                        framebuffer_width = fb_inf->width;
                        framebuffer_pitch = fb_inf->pitch;
                    }
                }
                break;
            }
            default:
                printf("  unsupported mb2 tag type: %d\n", tag->type);
                break;
        }

        // Iterate to the next tag
        uintptr_t next = ((uintptr_t)tag + tag->size + 7) & ~7ULL;
        tag = (multiboot_tag_t*)next;
    }

    // If we get here without finding END tag, still process memory map
    multiboot2_process_memory_map();
}

uint32_t multiboot2_get_max_memory()
{
    return pmm_max_memory;
}

uint32_t multiboot2_get_mmap_count()
{
    return saved_mmap_count;
}

const saved_mmap_entry_t* multiboot2_get_mmap_entry(uint32_t index)
{
    if (index >= saved_mmap_count) {
        return NULL;
    }
    return &saved_mmap[index];
}

void multiboot2_print_data()
{
    printf("Bootloader name: %s\n", boot_loader_name);
    printf("Boot command: %s\n", boot_command_line);
    printf("Load base address: %#x\n", load_base_addr);
    puts("Memory map:");
    for (uint32_t i = 0; i < multiboot2_get_mmap_count(); i++) {
        const saved_mmap_entry_t* entry = multiboot2_get_mmap_entry(i);
        char* type = "????";
        switch (entry->type) {
            case 1:
                type = "avlb"; // Available
                break;
            case 2:
                type = "rsvd"; // Reserved
                break;
            case 3:
                type = "acpi"; // ACPI reclaimable
                break;
            case 4:
                type = " nvs"; // NVS
                break;
            case 5:
                type = "bram"; // bad RAM
                break;
        }

        uint64_t base = ((uint64_t)entry->base_high << 32) |
                       entry->base_low;
        uint64_t length = ((uint64_t)entry->length_high << 32) |
                         entry->length_low;
        uint64_t end = base + length;
        printf(
            "  [%03d] %s %#016llx : %#016llx (%d KiB)\n",
            i,
            type,
            base,
            end,
            length / 1024
        );
    }
    puts("Boot modules data:");
    for (uint32_t i = 0; i < saved_boot_modules_count; i++) {
        uint32_t start = boot_modules[i].module_phys_addr_start;
        uint32_t end = boot_modules[i].module_phys_addr_end;
        printf(
            "  [%02d] >%s< %#x : %#x (len: %u KiB)\n",
            i,
            boot_module_names[i],
            PHYS_TO_VIRT(start),
            PHYS_TO_VIRT(end),
            (end - start) / 1024
        );
    }
}

char* multiboot2_get_boot_command_line()
{
    return boot_command_line;
}

uint32_t multiboot2_get_modules_count()
{
    return saved_boot_modules_count;
}

multiboot_tag_boot_module_t* multiboot2_get_boot_module_entry(uint32_t i) {
    if (i >= saved_boot_modules_count) {
        return NULL;
    }
    return &boot_modules[i];
}

char* multiboot2_get_boot_module_name(uint32_t i) {
    if (i >= saved_boot_modules_count) {
        return NULL;
    }
    return boot_module_names[i];
}

void multiboot2_load_framebuffer_rgb_config(framebuffer_rgb_config_t* res)
{
    res->enabled = rgb_mode;
    res->framebuffer_addr_phys = framebuffer_addr_phys;
    res->framebuffer_pitch = framebuffer_pitch;
    res->framebuffer_height = framebuffer_height;
    res->framebuffer_width = framebuffer_width;
}
