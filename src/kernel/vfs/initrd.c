#include "kernel/memory/pmm.h"
#include "kernel/multiboot2.h"
#include "kernel/paging.h"
#include <kernel/vfs/initrd.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static vfs_node_t* initrd = NULL;
static bool mounted = false;
static char empty_name[100];

/**
 * Iterates over file in TAR. Loads next TAR header to `next`.
 */
static void initrd_load_tar(tar_header_t* tar_header, tar_header_t** next)
{
    if (memcmp(tar_header->filename, empty_name, 100) == 0)
    {
        *next = NULL;
        return;
    }

    tar_header->filename[99] = '\0';
    uint32_t size_bytes = strtol(tar_header->size, NULL, 8);
    puts(tar_header->filename);
    printf("Size: %d B\n", size_bytes);

    char entry[16];
    strncpy(entry, ((char*)tar_header) + 512, 16);
    entry[15] = 0;
    puts(entry);

    uint32_t size_aligned = size_bytes;
    if (size_aligned % 512 != 0) {
        size_aligned += 512 - (size_aligned % 512);
    }
    *next = (tar_header_t*)(((char*)tar_header) + 512 + size_aligned);
}

vfs_node_t* initrd_mount()
{
    if (mounted) {
        return initrd;
    }
    mounted = true;
    puts("Loading initrd from memory");
    memset(empty_name, 0, 100);

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

    do {
        initrd_load_tar(tar_header, &tar_header);
    } while (tar_header != NULL);

    return initrd;
}
