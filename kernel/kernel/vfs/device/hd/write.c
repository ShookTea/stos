#include <string.h>
#include "./hd.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/drivers/ata.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

size_t hd_write(vfs_file_t* file, size_t offset, size_t size, const void* ptr)
{
    if (!file->writeable) {
        return 0;
    }

    hd_metadata_t* meta = file->dentry->inode->metadata;

    size_t wait_idx = rwq_allocate_pos(&hd_rw_queue);
    hd_wakeup_data_t wakeup_data;
    wakeup_data.wait_idx = wait_idx;
    wakeup_data.hd_metadata = meta;

    _debug_printf(
        "Received WRITE call for offset=%u size=%u, wait_idx=%u\n",
        offset,
        size,
        wait_idx
    );

    hd_sector_location_t loc;
    hd_calc_sector_loc(&loc, offset, size, meta);
    if (loc.size == 0) {
        rwq_deallocate_pos(&hd_rw_queue, wait_idx);
        return 0;
    }

    _debug_printf(
        "[wait_idx=%u] lba=%u, sector count=%u\n",
        wait_idx,
        loc.low_sector_lba,
        loc.sector_count
    );

    uint16_t* buffer = kmalloc_flags(
        loc.sector_size * loc.sector_count,
        KMALLOC_ZERO
    );

    if (offset != loc.lowest_sector_byte || size % loc.sector_size != 0) {
        _debug_printf(
            "[wait_idx=%u] not aligned to sectors - reading wait started\n",
            wait_idx
        );
        ata_read(
            meta->disk_id,
            loc.low_sector_lba,
            loc.sector_count,
            buffer,
            hd_rw_ready,
            &wakeup_data
        );
        wait_on_condition(meta->wait_obj, hd_rw_wait_for_ready, &wakeup_data);

        _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);
        rwq_reset_to_not_ready(&hd_rw_queue, wait_idx);
    }

    size_t offset_in_buffer = offset - loc.lowest_sector_byte;
    memcpy(((uint8_t*)buffer) + offset_in_buffer, ptr, loc.size);

    ata_write(
        meta->disk_id,
        loc.low_sector_lba,
        loc.sector_count,
        buffer,
        hd_rw_ready,
        &wakeup_data
    );

    _debug_printf("[wait_idx=%u] start for write\n", wait_idx);

    wait_on_condition(meta->wait_obj, hd_rw_wait_for_ready, &wakeup_data);

    _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);

    rwq_deallocate_pos(&hd_rw_queue, wait_idx);

    kfree(buffer);
    return loc.size;
}
