#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER

#include <stdint.h>

// Magic number that confirms that multiboot2 info was loaded correctly
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

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
} multiboot_tag_t;

/**
 * The whole multiboot structure starts with a total_size in bytes (including
 * the field) and the reserved block which should be ignored. It's terminated
 * by tag with type=0, size=8
 */
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
    multiboot_tag_t tags[0];
} multiboot_info_t;

#endif
