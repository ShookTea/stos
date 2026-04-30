#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "kernel/drivers/ata.h"
#include "./hd.h"

// on_release for dynamically-allocated partition nodes.
static void partition_node_release(vfs_node_t* node)
{
    hd_metadata_t* meta = node->metadata;
    if (meta != NULL) {
        if (meta->wait_obj != NULL) {
            wait_deallocate(meta->wait_obj);
        }
        kfree(meta);
    }
    kfree(node);
}

/**
 * Called by device.c `finddir` for names not matching static devices. Reads
 * current MBR on demand, and returns a dynamically-allocated node with an
 * `on_release` set so it is freed when the last file handle is closed.
 */
vfs_node_t* device_hd_finddir_partition(const char* name)
{
    if (hd_partition_wait_obj == NULL) {
        return NULL;
    }

    // Accept names of the form "hd[a-z][1-9]"
    if (name[0] != 'h' || name[1] != 'd'
        || name[2] < 'a' || name[2] > 'z'
        || name[3] < '1' || name[3] > '9'
        || name[4] != '\0') {
        return NULL;
    }
    char drive_letter = name[2];
    uint8_t part_num = (uint8_t)(name[3] - '1'); // 0-based

    uint8_t avail[5];
    ata_get_available_drives(avail);
    char dl = 'a';

    for (uint8_t* d = avail; *d != ATA_DRIVE_NONE; d++) {
        ata_disk_info_t di;
        ata_load_disk_info(*d, &di);

        if (di.type != PIO) {
            continue;
        }

        if (dl != drive_letter) {
            dl++;
            continue;
        }

        uint16_t* mbr_buf = kmalloc_flags(di.sector_size, KMALLOC_ZERO);
        hd_read_mbr_sync(*d, mbr_buf);

        ata_partition_t parts[4];
        uint8_t count = ata_parse_mbr_partitions((uint8_t*)mbr_buf, parts);
        kfree(mbr_buf);

        if (part_num >= count) {
            return NULL;
        }

        ata_partition_t* part = &parts[part_num];

        vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
        vfs_populate_node(node, (char*)name, VFS_TYPE_BLOCK_DEVICE);
        node->read_node = hd_read;
        node->write_node = hd_write;
        node->length = (uint64_t)part->sectors_count * di.sector_size;
        node->on_release = partition_node_release;

        hd_metadata_t* metadata = kmalloc_flags(
            sizeof(hd_metadata_t),
            KMALLOC_ZERO
        );
        metadata->disk_id = *d;
        metadata->is_partition = true;
        metadata->part_lba_start = part->lba_start;
        metadata->part_sectors_count = part->sectors_count;
        metadata->wait_obj = wait_allocate_queue();
        node->metadata = metadata;

        return node;
    }

    return NULL;
}
