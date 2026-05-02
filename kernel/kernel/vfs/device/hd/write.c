#include <string.h>
#include "./hd.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/drivers/ata.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

static void read_uncached_sectors(
    hd_metadata_t* meta,
    hd_wakeup_data_t* wakeup_data,
    uint8_t* buffer,
    size_t base_lba,
    size_t first_i,
    size_t last_i,
    size_t sector_size
) {
    size_t sector_start = base_lba + first_i;
    size_t sector_end = base_lba + last_i;

    ata_read(
        meta->disk_id,
        sector_start,
        sector_end - sector_start + 1,
        (uint16_t*)(buffer + first_i * sector_size),
        hd_rw_ready,
        wakeup_data
    );
    wait_on_condition(meta->wait_obj, hd_rw_wait_for_ready, wakeup_data);

    for (size_t lba = sector_start; lba <= sector_end; lba++) {
        hd_cache_upsert(
            meta->disk_id,
            lba,
            sector_size,
            buffer + (lba - base_lba) * sector_size,
            false
        );
    }
}

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

    uint8_t* buffer = kmalloc_flags(
        loc.sector_size * loc.sector_count,
        KMALLOC_ZERO
    );

    if (offset != loc.lowest_sector_byte || size % loc.sector_size != 0) {
        _debug_printf(
            "[wait_idx=%u] not aligned to sectors - pre-reading\n",
            wait_idx
        );

        bool prev_cache_hit = true;
        size_t first_uncached = 0;

        for (size_t i = 0; i < loc.sector_count; i++) {
            size_t sector = loc.low_sector_lba + i;
            bool cache_hit = hd_cache_seek(
                meta->disk_id,
                sector,
                buffer + i * loc.sector_size
            );

            if (!cache_hit) {
                if (prev_cache_hit) {
                    first_uncached = i;
                }
                prev_cache_hit = false;
                continue;
            }

            _debug_printf(
                "[wait_idx=%u] Cache hit for sector %u\n",
                wait_idx,
                sector
            );

            if (prev_cache_hit) {
                continue;
            }

            read_uncached_sectors(
                meta, &wakeup_data, buffer,
                loc.low_sector_lba, first_uncached, i - 1,
                loc.sector_size
            );
            prev_cache_hit = true;
        }

        if (!prev_cache_hit) {
            read_uncached_sectors(
                meta, &wakeup_data, buffer,
                loc.low_sector_lba, first_uncached, loc.sector_count - 1,
                loc.sector_size
            );
        }

        _debug_printf("[wait_idx=%u] pre-read completed\n", wait_idx);
    }

    size_t offset_in_buffer = offset - loc.lowest_sector_byte;
    memcpy(buffer + offset_in_buffer, ptr, loc.size);

    for (size_t i = 0; i < loc.sector_count; i++) {
        hd_cache_upsert(
            meta->disk_id,
            loc.low_sector_lba + i,
            loc.sector_size,
            buffer + i * loc.sector_size,
            true
        );
    }

    rwq_deallocate_pos(&hd_rw_queue, wait_idx);
    kfree(buffer);
    return loc.size;
}
