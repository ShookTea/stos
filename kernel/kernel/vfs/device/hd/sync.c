#include "./hd.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/drivers/ata.h"
#include "kernel/memory/kmalloc.h"

#define _debug_puts(...) debug_puts_cc(DBC_VFS_DEV, "hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_cc(DBC_VFS_DEV, "hd", __VA_ARGS__)

typedef struct {
    hd_wakeup_data_t wakeup_data;
    hd_metadata_t* meta;
} callback_arg_t;

static void hd_sync_callback(hd_cache_entry_t* entry, void* _arg)
{
    callback_arg_t* arg = _arg;
    hd_wakeup_data_t wakeup_data = arg->wakeup_data;
    hd_metadata_t* meta = arg->meta;

    size_t wait_idx = rwq_allocate_pos(&hd_rw_queue);
    wakeup_data.wait_idx = wait_idx;

    _debug_printf(
        "[sync] Writing dirty sector %u to disk [wait_idx=%u]\n",
        entry->sector_lba,
        wait_idx
    );

    ata_write(
        meta->disk_id,
        entry->sector_lba,
        1,
        (uint16_t*)entry->content,
        hd_rw_ready,
        &wakeup_data
    );
    wait_on_condition(meta->wait_obj, hd_rw_wait_for_ready, &wakeup_data);
    rwq_deallocate_pos(&hd_rw_queue, wait_idx);
    entry->is_dirty = false;
}

void hd_sync_with_metadata(hd_metadata_t* meta)
{
    hd_wakeup_data_t wakeup_data;
    wakeup_data.hd_metadata = meta;
    callback_arg_t* callback_arg = kmalloc(sizeof(callback_arg_t));
    callback_arg->meta = meta;
    callback_arg->wakeup_data = wakeup_data;

    hd_cache_iterate_dirty(meta->disk_id, hd_sync_callback, callback_arg);

    kfree(callback_arg);
    hd_cache_clear(meta->disk_id);
}

void hd_sync(const vfs_file_t* file)
{
    hd_metadata_t* meta = file->dentry->inode->metadata;
    hd_sync_with_metadata(meta);
}
