#include "kernel/pmm.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <kernel/multiboot2.h>

static multiboot_info_t* mbi;
static char boot_command_line[64];
static char boot_loader_name[64];
static uint32_t load_base_addr = 0;

void multiboot2_init(multiboot_info_t* multiboot_info)
{
    mbi = multiboot_info;
    uint64_t mb2_end = mbi->total_size + (uint32_t)mbi;
    // adding 8 bytes here to adjust for the multiboot_info_t header
    multiboot_tag_t* tag = (multiboot_tag_t*)((uint8_t*)mbi + 8);

    while ((uint32_t)tag < mb2_end) {
        switch (tag->type) {
            case MULTIBOOT2_TAG_TYPE_END:
                // We've reached the END tag
                return;
            case MULTIBOOT2_TAG_TYPE_BOOT_COMMAND_LINE: {
                multiboot_tag_boot_command_line_t* cl =
                    (multiboot_tag_boot_command_line_t*)tag;
                size_t len = strlen(cl->command_line);
                if (len > 64) len = 64;
                memcpy(boot_command_line, cl->command_line, len);
                break;
            }
            case MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME: {
                multiboot_tag_boot_loader_name_t* bln =
                    (multiboot_tag_boot_loader_name_t*)tag;
                size_t len = strlen(bln->name);
                if (len > 64) len = 64;
                memcpy(boot_loader_name, bln->name, len);
                break;
            }
            case MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR:
                load_base_addr = ((multiboot_tag_load_base_addr_t*)tag)
                    ->load_base_addr;
                break;
            case MULTIBOOT2_TAG_TYPE_MEMORY_MAP:
                // printf(format, index, tag->type, "memory_map");
                // multiboot_tag_memory_map_t* mmap =
                //     (multiboot_tag_memory_map_t*)tag;
                // uint8_t entries_count = (mmap->size - 8) / mmap->entry_size;
                // printf(" (count = %d)\n", entries_count);
                // uint8_t mem_index;
                // for (mem_index = 0; mem_index < entries_count; mem_index++) {
                //     multiboot_tag_memory_map_entry_t mmap_entry =
                //         mmap->entries[mem_index];
                //     char* entry_type;
                //     switch (mmap_entry.type) {
                //         case 1:
                //             entry_type = "avail. RAM";
                //             break;
                //         case 2:
                //             entry_type = "reserved";
                //             break;
                //         case 3:
                //             entry_type = "ACPI-reclaimable";
                //             break;
                //         case 4:
                //             entry_type = "NVS";
                //             break;
                //         case 5:
                //             entry_type = "defective RAM";
                //             break;
                //         default:
                //             entry_type = "unrecognized";
                //     }
                //     printf(
                //         "  %2d: %-16s (%0#8x%08x:%0#8x%08x)\n",
                //         mem_index,
                //         entry_type,
                //         mmap_entry.base_addr_high,
                //         mmap_entry.base_addr_low,
                //         mmap_entry.length_high,
                //         mmap_entry.length_low
                //     );
                // }
                break;
            default:
                // Unknown multiboot2 mode
                break;
        }

        // Iterate to the next tag
        uintptr_t next = ((uintptr_t)tag + tag->size + 7) & ~7ULL;
        tag = (multiboot_tag_t*)next;
    }
}

void multiboot2_print_stats()
{
    printf("Bootloader name: %s\n", boot_loader_name);
    printf("Boot command: %s\n", boot_command_line);
    printf("Load base address: %#x\n", load_base_addr);
}
