#include "./hd.h"
#include "kernel/memory/kmalloc.h"
#include <string.h>

/**
 * Two-dimensional array. First dimension is disk ID, second dimension grows
 * on request.
 */
static hd_cache_entry_t** cache_entries = NULL;
static size_t* cache_count_per_disk = NULL;
static size_t disk_count = 0;

/**
 * If given sector at a disk exists, copy it's content to `buf` and return
 * `true`.
 */
bool hd_cache_seek(const uint8_t disk_id, const size_t lba, uint8_t* buf)
{
    // Cache not initialized
    if (disk_count < ((size_t)disk_id + 1)) {
        return false;
    }

    hd_cache_entry_t* entries = cache_entries[disk_id];
    for (size_t i = 0; i < cache_count_per_disk[disk_id]; i++) {
        if (entries[i].sector_lba == lba) {
            memcpy(
                buf,
                entries[i].content,
                entries[i].sector_size
            );
            return true;
        }
    }

    // LBA not found
    return false;
}

void hd_cache_load(
    const uint8_t disk_id,
    const size_t lba,
    const size_t sector_size,
    const uint8_t* buf
) {
    if (disk_count < ((size_t)disk_id + 1)) {
        // Cache not initialized for given disk ID
        size_t new_disk_count = disk_id + 1;
        cache_entries = krealloc(
            cache_entries,
            sizeof(hd_cache_entry_t*) * new_disk_count
        );
        cache_count_per_disk = krealloc(
            cache_count_per_disk,
            sizeof(size_t) * new_disk_count
        );
        for (size_t i = disk_count; i < new_disk_count; i++) {
            cache_entries[i] = NULL;
            cache_count_per_disk[i] = 0;
        }

        disk_count = new_disk_count;
    }

    // Add entry
    size_t index = cache_count_per_disk[disk_id];
    cache_entries[disk_id] = krealloc(
        cache_entries[disk_id],
        sizeof(hd_cache_entry_t) * (index + 1)
    );
    // return;
    cache_entries[disk_id][index].sector_size = sector_size;
    cache_entries[disk_id][index].sector_lba = lba;
    cache_entries[disk_id][index].is_dirty = false;
    cache_entries[disk_id][index].content = kmalloc(sector_size);
    memcpy(
        cache_entries[disk_id][index].content,
        buf,
        sector_size
    );

    cache_count_per_disk[disk_id]++;
}

void hd_cache_update(
    const uint8_t disk_id,
    const size_t lba,
    const uint8_t* buf
) {
    // Cache not initialized
    if (disk_count < ((size_t)disk_id + 1)) {
        return;
    }

    hd_cache_entry_t* entries = cache_entries[disk_id];
    for (size_t i = 0; i < cache_count_per_disk[disk_id]; i++) {
        if (entries[i].sector_lba == lba) {
            memcpy(
                entries[i].content,
                buf,
                entries[i].sector_size
            );
            entries[i].is_dirty = true;
            return;
        }
    }
}
