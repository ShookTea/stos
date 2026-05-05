#include "./hd.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/drivers/ata.h"

#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

void hd_sync_with_metadata(hd_metadata_t* meta)
{
    size_t count;
    hd_cache_entry_t* entries = hd_cache_get_entries(meta->disk_id, &count);
    if (count == 0) {
        return;
    }

    size_t wait_idx = rwq_allocate_pos(&hd_rw_queue);
    hd_wakeup_data_t wakeup_data;
    wakeup_data.wait_idx = wait_idx;
    wakeup_data.hd_metadata = meta;

    for (size_t i = 0; i < count; i++) {
        if (!entries[i].is_dirty) {
            continue;
        }

        _debug_printf(
            "[sync] Writing dirty sector %u to disk\n",
            entries[i].sector_lba
        );

        ata_write(
            meta->disk_id,
            entries[i].sector_lba,
            1,
            (uint16_t*)entries[i].content,
            hd_rw_ready,
            &wakeup_data
        );
        wait_on_condition(meta->wait_obj, hd_rw_wait_for_ready, &wakeup_data);
    }

    rwq_deallocate_pos(&hd_rw_queue, wait_idx);
    hd_cache_clear(meta->disk_id);
}

void hd_sync(const vfs_file_t* file)
{
    hd_metadata_t* meta = file->dentry->inode->metadata;
    hd_sync_with_metadata(meta);
}
