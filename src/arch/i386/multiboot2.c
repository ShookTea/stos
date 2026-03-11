#include <stdio.h>
#include <stdbool.h>
#include <kernel/multiboot2.h>

static multiboot_info_t* mbi;

void multiboot2_init(multiboot_info_t* multiboot_info)
{
    mbi = multiboot_info;
}


void multiboot2_print_debug_info()
{
    puts("== GRUB MULTIBOOT2 DEBUG INFO==");
    printf("Total multiboot2 size: %d B\n", mbi->total_size);
    uint8_t index = 0;
    uint64_t mb2_end = mbi->total_size + (uint32_t)mbi;
    // adding 8 bytes here to adjust for the multiboot_info_t header
    multiboot_tag_t* tag = (multiboot_tag_t*)((uint8_t*)mbi + 8);

    char* format = "[%2d] #%02u:%s";
    while ((uint32_t)tag < mb2_end) {
        switch (tag->type) {
            case MULTIBOOT2_TAG_TYPE_END:
                printf(format, index, tag->type, "end");
                puts("");
                return;  // Exit when we hit the END tag
            case MULTIBOOT2_TAG_TYPE_BOOT_COMMAND_LINE:
                printf(format, index, tag->type, "command_line");
                printf(
                    " (%s)\n",
                    ((multiboot_tag_boot_command_line_t*)tag)->command_line
                );
                break;
            case MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME:
                printf(format, index, tag->type, "boot_loader_name");
                printf(
                    " (%s)\n",
                    ((multiboot_tag_boot_loader_name_t*)tag)->name
                );
                break;
            case MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR:
                printf(format, index, tag->type, "load_base_addr");
                printf(
                    " (%#x)\n",
                    ((multiboot_tag_load_base_addr_t*)tag)->load_base_addr
                );
                break;
            case MULTIBOOT2_TAG_TYPE_APM_TABLE:
                printf(format, index, tag->type, "apm_table");
                multiboot_tag_apm_table_t* apm =
                    (multiboot_tag_apm_table_t*)tag;
                printf(
                    "(fl=%#04x;off=%#04x)\n",
                    apm->flags,
                    apm->offset
                );
                break;
            case MULTIBOOT2_TAG_TYPE_MEMORY_MAP:
                printf(format, index, tag->type, "memory_map");
                multiboot_tag_memory_map_t* mmap =
                    (multiboot_tag_memory_map_t*)tag;
                uint8_t entries_count = (mmap->size - 8) / mmap->entry_size;
                printf(" (count = %d)\n", entries_count);
                uint8_t mem_index;
                for (mem_index = 0; mem_index < entries_count; mem_index++) {
                    multiboot_tag_memory_map_entry_t mmap_entry =
                        mmap->entries[mem_index];
                    char* entry_type;
                    switch (mmap_entry.type) {
                        case 1:
                            entry_type = "avail. RAM";
                            break;
                        case 2:
                            entry_type = "reserved";
                            break;
                        case 3:
                            entry_type = "ACPI-reclaimable";
                            break;
                        case 4:
                            entry_type = "NVS";
                            break;
                        case 5:
                            entry_type = "defective RAM";
                            break;
                        default:
                            entry_type = "unrecognized";
                    }
                    printf(
                        "  %2d: %-16s (%0#8x%08x:%0#8x%08x)\n",
                        mem_index,
                        entry_type,
                        mmap_entry.base_addr_high,
                        mmap_entry.base_addr_low,
                        mmap_entry.length_high,
                        mmap_entry.length_low
                    );
                }
                break;
            default:
                printf(format, index, tag->type, "!unknown!");
                puts("");
                break;
        }

        index++;
        // Iterate to the next tag
        uintptr_t next = ((uintptr_t)tag + tag->size + 7) & ~7ULL;
        tag = (multiboot_tag_t*)next;
    }
}
