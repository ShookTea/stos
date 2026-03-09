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
    uint64_t mb2_end = mbi->total_size + (uint64_t)mbi;
    // adding 8 bytes here to adjust for the multiboot_info_t header
    multiboot_tag_t* tag = (multiboot_tag_t*)((uint8_t*)mbi + 8);

    char* format = "[%2d] #%02u:%s";
    while ((uint64_t)tag < mb2_end) {
        switch (tag->type) {
            case MULTIBOOT2_TAG_TYPE_END:
                printf(format, index, tag->type, "end");
                puts("");
                break;
            case MULTIBOOT2_TAG_TYPE_BOOT_COMMAND_LINE:
                printf(format, index, tag->type, "command_line");
                printf(
                    " (%s)\n",
                    ((multiboot_tag_boot_command_line_t*)tag)->command_line
                );
                break;
            case MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR:
                printf(format, index, tag->type, "load_base_addr");
                printf(
                    " (%#x)\n",
                    ((multiboot_tag_load_base_addr_t*)tag)->load_base_addr
                );
                break;
            default:
                printf(format, index, tag->type, "!unkown!");
                puts("");
                break;
        }
        // printf("--TAG #%3d--\n", index);
        // printf("  tag type: (%u)\n", tag->type);
        // printf("  size: %uB\n", tag->size);

        // if (tag->type == 1) {
        //     multiboot_tag_boot_command_line_t* mapped = (multiboot_tag_boot_command_line_t*)tag;
        //     printf("  line: %s\n", mapped->command_line);
        // }

        index++;
        // Iterate to the next tag
        uintptr_t next = ((uintptr_t)tag + tag->size + 7) & ~7ULL;
        tag = (multiboot_tag_t*)next;
    }
}
