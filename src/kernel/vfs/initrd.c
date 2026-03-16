#include "kernel/multiboot2.h"
#include "kernel/paging.h"
#include <kernel/vfs/initrd.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static vfs_node_t* initrd = NULL;
static bool mounted = false;

vfs_node_t* initrd_mount()
{
    if (mounted) {
        return initrd;
    }
    mounted = true;
    puts("Loading initrd from memory");

    multiboot_tag_boot_module_t* initrd_module = NULL;
    for (uint32_t i = 0; i < multiboot2_get_modules_count(); i++) {
        if (strcmp("initrd", multiboot2_get_boot_module_name(i)) == 0) {
            initrd_module = multiboot2_get_boot_module_entry(i);
            break;
        }
    }

    if (!initrd_module) {
        puts("There is no initrd module present.");
        return NULL;
    }

    tar_header_t* tar_header = PHYS_TO_VIRT(
        initrd_module->module_phys_addr_start
    );

    tar_header->filename[99] = '\0';
    puts(tar_header->filename);

    return initrd;
}
