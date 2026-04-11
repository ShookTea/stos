#include "../device.h"
#include "kernel/debug.h"
#include "kernel/drivers/ata.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

typedef struct {
    // one of ATA_DRIVE_ values
    uint8_t disk_id;
    // Tasks queue
    wait_obj_t* wait_obj;
} hd_metadata_t;

static vfs_node_t** nodes = NULL;

vfs_node_t** device_hd_mount()
{
    if (nodes != NULL) {
        return nodes;
    }

    // 4 pointers + 1 terminating NULL pointer
    nodes = kmalloc_flags(sizeof(vfs_node_t*) * 5, KMALLOC_ZERO);

    _debug_puts("Mounting HD files to VFS");
    uint8_t ata_drives[5];
    ata_get_available_drives(ata_drives);
    uint8_t* ata_drive_ptr = ata_drives;
    char* drive_name = "hda";
    char drive_letter = 'a';
    size_t pointer_index = 0;
    while (*ata_drive_ptr != 0) {
        drive_name[2] = drive_letter;
        _debug_printf(
            "drive with type %d found, mounting to /dev/%s\n",
            *ata_drive_ptr,
            drive_name
        );

        vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
        vfs_populate_node(node, drive_name, VFS_TYPE_BLOCK_DEVICE);
        hd_metadata_t* metadata = kmalloc_flags(
            sizeof(hd_metadata_t),
            KMALLOC_ZERO
        );
        node->metadata = metadata;
        metadata->disk_id = *ata_drive_ptr;
        metadata->wait_obj = wait_allocate_queue();
        nodes[pointer_index] = node;

        drive_letter++;
        ata_drive_ptr++;
        pointer_index++;
    }

    return nodes;
}

void device_hd_unmount()
{
    if (nodes != NULL) {
        vfs_node_t** ptr = nodes;
        while (*ptr != NULL) {
            vfs_node_t* node = *ptr;
            hd_metadata_t* metadata = node->metadata;
            if (metadata != NULL) {
                if (metadata->wait_obj != NULL) {
                    kfree(metadata->wait_obj);
                }
                kfree(metadata);
            }
            kfree(node);
            ptr++;
        }
        kfree(nodes);
    }
}
