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
bool hd_cache_seek(uint8_t disk_id, size_t lba, uint16_t* buf)
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
