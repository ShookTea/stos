#include <string.h>
#include "./hd.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/drivers/ata.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

size_t hd_read(vfs_file_t* file, size_t offset, size_t size,void* ptr)
{
    if (!file->readable) {
        return 0;
    }

    hd_metadata_t* meta = file->dentry->inode->metadata;

    size_t wait_idx = rwq_allocate_pos(&hd_rw_queue);
    hd_wakeup_data_t wakeup_data;
    wakeup_data.wait_idx = wait_idx;
    wakeup_data.hd_metadata = meta;

    _debug_printf(
        "Received READ call for offset=%u size=%u, wait_idx=%u\n",
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

    bool prev_cache_hit = true;
    size_t first_uncached_sector = 0;
    for (size_t i = 0; i < loc.sector_count; i++)
    {
        size_t sector = loc.low_sector_lba + i;
        // Try to read data to the buffer from cache
        bool cache_hit = hd_cache_seek(
            meta->disk_id,
            sector,
            buffer + (i * loc.sector_size)
        );

        if (!cache_hit) {
            // Current sector is not in cache
            if (prev_cache_hit) {
                // this is the first sector in a row that is not cached - save
                // it for later
                first_uncached_sector = i;
            }
            prev_cache_hit = false;
            continue;
        }

        // Sector was in cache and is now in the buffer.
        _debug_printf("[wait_idx=%u] Cache hit for sector %u\n", sector);

        if (prev_cache_hit) {
            // Previous sector was also in the cache
            continue;
        }

        // Previous sector(s) weren't in the cache and need to be actually
        // loaded from ATA.
        size_t sector_start = loc.low_sector_lba + first_uncached_sector;
        size_t sector_end = loc.low_sector_lba + i - 1;
        _debug_printf(
            "[wait_idx=%u] Reading data from ATA for sectors %u-%u\n",
            sector_start,
            sector_end
        );

        ata_read(
            meta->disk_id,
            sector_start,
            sector_end - sector_start + 1,
            buffer + (first_uncached_sector * loc.sector_size),
            hd_rw_ready,
            &wakeup_data
        );

        _debug_printf("[wait_idx=%u] start waiting\n", wait_idx);
        wait_on_condition(meta->wait_obj, hd_rw_wait_for_ready, &wakeup_data);
        _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);

        // Clear flag
        prev_cache_hit = true;
    }

    if (!prev_cache_hit) {
        ata_read(
            meta->disk_id,
            loc.low_sector_lba + first_uncached_sector,
            loc.sector_count - first_uncached_sector,
            buffer + (first_uncached_sector * loc.sector_size),
            hd_rw_ready,
            &wakeup_data
        );

        _debug_printf("[wait_idx=%u] start waiting\n", wait_idx);
        wait_on_condition(meta->wait_obj, hd_rw_wait_for_ready, &wakeup_data);
        _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);
    }

    size_t offset_in_buffer = offset - loc.lowest_sector_byte;
    memcpy(ptr, ((uint8_t*)buffer) + offset_in_buffer, loc.size);
    rwq_deallocate_pos(&hd_rw_queue, wait_idx);

    kfree(buffer);
    return loc.size;
}
