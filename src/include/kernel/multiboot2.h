#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER

#include <stdint.h>

// Magic number that confirms that multiboot2 info was loaded correctly
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT2_TAG_TYPE_END 0
#define MULTIBOOT2_TAG_TYPE_BOOT_COMMAND_LINE 1
#define MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT2_TAG_TYPE_MEMORY_MAP 6
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

/**
 * Store multiboot2 information
 */
void multiboot2_init(multiboot_info_t*);

void multiboot2_print_stats();

#endif
