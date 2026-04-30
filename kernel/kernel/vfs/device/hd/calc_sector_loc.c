#include "kernel/debug.h"
#include "./hd.h"
#include "kernel/drivers/ata.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

static inline size_t sector_align_down(size_t addr, size_t sector_size)
{
    return addr & ~(sector_size - 1);
}

static inline size_t sector_align_up(size_t addr, size_t sector_size)
{
    return (addr + sector_size - 1) & ~(sector_size - 1);
}

void hd_calc_sector_loc(
    hd_sector_location_t* loc,
    size_t offset,
    size_t size,
    hd_metadata_t* meta
) {
    ata_disk_info_t disk_info;
    ata_load_disk_info(meta->disk_id, &disk_info);
    size_t sector_size = disk_info.sector_size;

    size_t lowest_sector_byte = sector_align_down(offset, sector_size);
    size_t highest_sector_byte = sector_align_up(offset + size, sector_size);

    loc->size = size;
    loc->lowest_sector_byte = lowest_sector_byte;
    loc->sector_size = sector_size;

    if (!meta->is_partition) {
        size_t disk_sectors = disk_info.sectors_count;
        loc->low_sector_lba = lowest_sector_byte / sector_size;
        size_t high_sector_lba = highest_sector_byte / sector_size;

        if (loc->low_sector_lba >= disk_sectors) {
            loc->size = 0;
            return;
        }

        if (high_sector_lba > disk_sectors) {
            high_sector_lba = disk_sectors;
            loc->size = disk_sectors * sector_size - offset;
        }

        loc->sector_count = high_sector_lba - loc->low_sector_lba;
        return;
    }

    // Partition: use LBA and sector count stored directly in metadata.
    size_t low_sector_lba_in_part = lowest_sector_byte / sector_size;
    size_t high_sector_lba_in_part = highest_sector_byte / sector_size;
    size_t sector_count = high_sector_lba_in_part - low_sector_lba_in_part;

    if (low_sector_lba_in_part >= meta->part_sectors_count) {
        _debug_puts("Err on read: read start beyond partition border");
        loc->size = 0;
        return;
    }

    if (high_sector_lba_in_part > meta->part_sectors_count) {
        sector_count -= high_sector_lba_in_part - meta->part_sectors_count;
        loc->size = meta->part_sectors_count * sector_size - offset;
    }

    loc->low_sector_lba = low_sector_lba_in_part + meta->part_lba_start;
    loc->sector_count = sector_count;
}
