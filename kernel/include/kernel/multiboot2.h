#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER

#include <stdint.h>
#include <kernel/acpi.h>

// Magic number that confirms that multiboot2 info was loaded correctly
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT2_TAG_TYPE_END 0
#define MULTIBOOT2_TAG_TYPE_BOOT_COMMAND_LINE 1
#define MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT2_TAG_TYPE_MODULES 3
#define MULTIBOOT2_TAG_TYPE_MEMORY_MAP 6
#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER_INFO 8
#define MULTIBOOT2_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT2_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT2_TAG_TYPE_APM_TABLE 10
#define MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR 21

/**
 * The multiboot2 starts with a fixed part and a series of tags.
 */

/**
 * Each multiboot tag starts with a type and size. Tags are padded in order for
 * each tag to start at 8-bytes aligned address. The "size" includes the tag
 * header, but not that padding.
 */
typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot_tag_t;

/**
 * The whole multiboot structure starts with a total_size in bytes (including
 * the field) and the reserved block which should be ignored. It's terminated
 * by tag with type=0, size=8
 */
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
    multiboot_tag_t tags[0];
} __attribute__((packed)) multiboot_info_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    char command_line[0];
} multiboot_tag_boot_command_line_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    char name[0];
} multiboot_tag_boot_loader_name_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    rsdp_old_t rsdp;
} multiboot_tag_boot_acpi_old_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    rsdp_new_t rsdp;
} multiboot_tag_boot_acpi_new_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t module_phys_addr_start;
    uint32_t module_phys_addr_end;
    char module_name[];
} multiboot_tag_boot_module_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t framebuffer_addr_low;
    uint32_t framebuffer_addr_high;
    uint32_t pitch;
    uint32_t width; // width of framebuffer in pixels
    uint32_t height; // height of framebuffer in pixels
    uint8_t bpp; // bits per pixel
    /**
     * Type of color, which defines content immediatelly following the reserved
     * field:
     * - if color_type=0: indexed color (fb_info_indexed_t)
     * - if color_type=1: direct RGB (fb_info_rgb_t)
     * - if color_type=2: EGA text:
     *   - width and height are in characters instead of pixels
     *   - pitch is expressed in bytes per text line
     */
    uint8_t color_type;
    uint8_t reserved;
} multiboot_tag_framebuffer_info_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} fb_info_indexed_palette_t;

typedef struct {
    uint32_t palette_count;
    fb_info_indexed_palette_t palette[0];
} fb_info_indexed_t;

typedef struct {
    uint8_t red_field_pos;
    uint8_t red_mask_size;
    uint8_t green_field_pos;
    uint8_t green_mask_size;
    uint8_t blue_field_pos;
    uint8_t blue_mask_size;
} fb_info_rgb_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t load_base_addr;
} __attribute__((packed))  multiboot_tag_load_base_addr_t;

typedef struct {
    // TODO: is it first high, then low, or the other way around?
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
    uint32_t reserved;
}  __attribute__((packed)) multiboot_tag_memory_map_entry_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_tag_memory_map_entry_t entries[0];
} multiboot_tag_memory_map_t;

// Memory map entry structure (saved internally)
typedef struct {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} saved_mmap_entry_t;

/**
 * Store multiboot2 information
 */
void multiboot2_init(multiboot_info_t*);

/**
 * Get the maximum usable memory address (calculated from memory map)
 * @return Maximum physical memory address
 */
uint32_t multiboot2_get_max_memory(void);

/**
 * Get the number of memory map entries
 * @return Number of memory map entries
 */
uint32_t multiboot2_get_mmap_count(void);

/**
 * Get a specific memory map entry
 * @param index Index of the memory map entry
 * @return Pointer to the memory map entry, or NULL if index is out of bounds
 */
const saved_mmap_entry_t* multiboot2_get_mmap_entry(uint32_t index);

uint32_t multiboot2_get_modules_count(void);
multiboot_tag_boot_module_t* multiboot2_get_boot_module_entry(uint32_t);
char* multiboot2_get_boot_module_name(uint32_t);
void multiboot2_print_data(bool force_terminal_output);
char* multiboot2_get_boot_command_line();

// Virtual address where the framebuffer is mapped after paging is enabled
#define FRAMEBUFFER_VIRT_BASE 0xB0000000

typedef struct {
    bool enabled;
    uint32_t framebuffer_addr_phys;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t framebuffer_pitch;
} framebuffer_rgb_config_t;

/**
 * Loads configuration of VGA screen for RGB mode
 */
void multiboot2_load_framebuffer_rgb_config(framebuffer_rgb_config_t* addr);

#endif
