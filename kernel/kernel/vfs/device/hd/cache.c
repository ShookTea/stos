#include "./hd.h"
#include "kernel/memory/kmalloc.h"
#include <string.h>

#define STARTING_POPULARITY 10
#define POPULARITY_INCREMENT 1
#define POPULARITY_DECREMENT 1
#define POPULARITY_MAX 255
#define MAX_ENTRIES_PER_DISK 128

#define min(a, b) ((a < b) ? a : b)

/**
 * Heads and tails of lists, indexed by disk ID
 */
static hd_cache_entry_t** cache_entry_heads = NULL;
static hd_cache_entry_t** cache_entry_tails = NULL;
static size_t* cache_count_per_disk = NULL;
static size_t disk_count = 0;

static inline void popularity_inc(hd_cache_entry_t* entry)
{
    entry->popularity = min(
        entry->popularity + POPULARITY_INCREMENT,
        POPULARITY_MAX
    );
}

static inline void popularity_dec(hd_cache_entry_t* entry)
{
    if (entry->popularity <= POPULARITY_DECREMENT) {
        entry->popularity = 0;
        return;
    }
    entry->popularity -= POPULARITY_DECREMENT;
}

/**
 * Update location of passed entry based on popularity
 */
static void hd_cache_sort(size_t disk_id, hd_cache_entry_t* entry)
{
    while (entry->prev != NULL && entry->prev->popularity < entry->popularity) {
        hd_cache_entry_t* prev = entry->prev;
        if (prev->prev != NULL) {
            prev->prev->next = entry;
        }
        if (entry->next != NULL) {
            entry->next->prev = prev;
        }
        prev->next = entry->next;
        entry->prev = prev->prev;
        entry->next = prev;
        prev->prev = entry;

        if (entry == cache_entry_tails[disk_id]) {
            cache_entry_tails[disk_id] = prev;
        }
        if (prev == cache_entry_heads[disk_id]) {
            cache_entry_heads[disk_id] = entry;
        }
    }
}

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

    hd_cache_entry_t* entry = cache_entry_heads[disk_id];
    while (entry != NULL) {
        if (entry->sector_lba == lba) {
            memcpy(
                buf,
                entry->content,
                entry->sector_size
            );
            popularity_inc(entry);
            hd_cache_sort(disk_id, entry);
            return true;
        }
        entry = entry->next;
    }

    // LBA not found
    return false;
}

void hd_cache_upsert(
    const uint8_t disk_id,
    const size_t lba,
    const size_t sector_size,
    const uint8_t* buf,
    const bool dirty
) {
    // Update existing entry if present
    if (disk_count >= ((size_t)disk_id + 1)) {
        hd_cache_entry_t* entry = cache_entry_heads[disk_id];
        while (entry != NULL) {
            if (entry->sector_lba == lba) {
                memcpy(entry->content, buf, entry->sector_size);
                entry->is_dirty = dirty;
                popularity_inc(entry);
                hd_cache_sort(disk_id, entry);
                return;
            }
            entry = entry->next;
        }
    }

    // Entry not found. First check if we need to setup disk slot
    if (disk_count < ((size_t)disk_id + 1)) {
        size_t new_disk_count = disk_id + 1;
        cache_entry_heads = krealloc(
            cache_entry_heads,
            sizeof(hd_cache_entry_t*) * new_disk_count
        );
        cache_entry_tails = krealloc(
            cache_entry_tails,
            sizeof(hd_cache_entry_t*) * new_disk_count
        );
        cache_count_per_disk = krealloc(
            cache_count_per_disk,
            sizeof(size_t) * new_disk_count
        );
        for (size_t i = disk_count; i < new_disk_count; i++) {
            cache_entry_heads[i] = NULL;
            cache_entry_tails[i] = NULL;
            cache_count_per_disk[i] = 0;
        }
        disk_count = new_disk_count;
    }

    // Allocate new entry for cache
    hd_cache_entry_t* new_entry = kmalloc_flags(
        sizeof(hd_cache_entry_t),
        KMALLOC_ZERO
    );
    new_entry->sector_size = sector_size;
    new_entry->sector_lba = lba;
    new_entry->is_dirty = dirty;
    new_entry->popularity = STARTING_POPULARITY;
    new_entry->content = kmalloc(sector_size);
    memcpy(new_entry->content, buf, sector_size);

    // Add entry to the end of the list
    new_entry->prev = cache_entry_tails[disk_id];
    if (cache_entry_tails[disk_id] != NULL) {
        cache_entry_tails[disk_id]->next = new_entry;
    }
    cache_entry_tails[disk_id] = new_entry;
    // Handle case where this entry is the first one
    if (cache_entry_heads[disk_id] == NULL) {
        cache_entry_heads[disk_id] = new_entry;
    }

    cache_count_per_disk[disk_id]++;
    hd_cache_sort(disk_id, new_entry);
}

void hd_cache_iterate_dirty(
    uint8_t disk_id,
    void (*callback)(hd_cache_entry_t* entry, void* arg),
    void* arg
) {
    if (disk_count < ((size_t)disk_id + 1)) {
        return;
    }

    hd_cache_entry_t* entry = cache_entry_heads[disk_id];
    while (entry != NULL) {
        if (entry->is_dirty) {
            callback(entry, arg);
        }
        entry = entry->next;
    }
}

void hd_cache_clear(uint8_t disk_id)
{
    // Cache not initialized
    if (disk_count < ((size_t)disk_id + 1)) {
        return;
    }

    size_t entries_count = cache_count_per_disk[disk_id];
    // The forced minimum number of items to be removed
    size_t forced_cut = (entries_count > MAX_ENTRIES_PER_DISK)
        ? (entries_count - MAX_ENTRIES_PER_DISK)
        : 0;

    hd_cache_entry_t* entry = cache_entry_tails[disk_id];
    while (entry != NULL) {
        popularity_dec(entry);
        hd_cache_entry_t* prev_entry = entry->prev;

        if (entry->popularity == 0 || forced_cut > 0) {
            if (prev_entry != NULL) {
                prev_entry->next = entry->next;
            }
            if (entry->next != NULL) {
                entry->next->prev = prev_entry;
            }
            if (cache_entry_tails[disk_id] == entry) {
                cache_entry_tails[disk_id] = prev_entry;
            }
            if (cache_entry_heads[disk_id] == entry) {
                cache_entry_heads[disk_id] = entry->next;
            }
            kfree(entry->content);
            kfree(entry);

            if (forced_cut > 0) {
                forced_cut--;
            }
            cache_count_per_disk[disk_id]--;
        }

        entry = prev_entry;
    }
}
