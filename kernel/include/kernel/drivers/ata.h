#ifndef KERNEL_DRIVERS_ATA_H
#define KERNEL_DRIVERS_ATA_H

#include <stdint.h>
#include <stdbool.h>

#define ATA_DRIVE_PRIMARY_MASTER 0b00
#define ATA_DRIVE_PRIMARY_SLAVE 0b01
#define ATA_DRIVE_SECONDARY_MASTER 0b10
#define ATA_DRIVE_SECONDARY_SLAVE 0b11
#define ATA_DRIVE_NONE 5

#define ata_drive_is_primary(b) ((b & 0b10) == 0)
#define ata_drive_is_master(b) ((b & 0b01) == 0)

#define ATA_PARTITION_TYPE_FAT32 0x0C
#define ATA_PARTITION_TYPE_LINUX_NATIVE 0x83

typedef enum {
    NOT_PRESENT = 0,
    PIO = 1,
    ATAPI = 2,
} ata_disk_type_t;

typedef struct {
    // One of ATA_PARTITION_TYPE_ values
    uint8_t type;
    // LBA of the start of the partition
    uint32_t lba_start;
    // Total number of sectors
    uint32_t sectors_count;
    // Is partition marked as bootable?
    bool bootable;
} ata_partition_t;

typedef struct {
    ata_disk_type_t type;
    // Null-terminated firmware name
    char firmware_name[40];
    // Total count of sectors
    uint32_t sectors_count;
    // Size of a single sector in bytes
    uint16_t sector_size;
} ata_disk_info_t;

/**
 * Parses MBR partition table from a raw sector buffer (512 bytes).
 * Populates out_parts (caller must provide space for at least 4 entries).
 * Returns the number of partitions found (0–4).
 */
uint8_t ata_parse_mbr_partitions(
    const uint8_t* mbr_buffer,
    ata_partition_t* out_parts
);

/**
 * Attempts to initialize ATA driver.
 */
void ata_init();

/**
 * Stores NULL-terminated list of available ATA_DRIVE_ values. Caller is
 * responsible for making sure that there's enough free memory available under
 * given address.
 */
void ata_get_available_drives(uint8_t*);

/**
 * Returns one of ATA_DRIVE_ values.
 */
uint8_t ata_get_selected_drive(void);

/**
 * Select a drive.
 */
void ata_select_drive(uint8_t);

/**
 * Loads disk information about selected disk to the pointer, or returns false
 * if disk doesn't exist.
 */
bool ata_load_disk_info(uint8_t disk_id, ata_disk_info_t* ptr);

/**
 * Create a read request for ATA driver, for currently selected drive.
 * - "ata_drive" is one of ATA_DRIVE_ values
 * - "lba" is the first sector from which reading will take place.
 * - "sectors_count" is the total number of sectors to be loaded.
 * - "buffer" is pre-allocated address in the memory where data will be stored.
 *   each sector is loaded as (sector_size / 2) 16-bit integers; thus the
 *   required memory is sector_size * sectors_count. It is the responsibility
 *   of the caller to make sure that there's enough memory to store data.
 * - "callback" is a function that will be called once the read operation is
 *   completed. "callback_data" will be passed as an argument.
 *
 * Returns "true" when the request was accepted.
 */
bool ata_read(
    uint8_t ata_drive,
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
);

/**
 * Create a write request for ATA driver, for currently selected drive.
 * - "ata_drive" is one of ATA_DRIVE_ values
 * - "lba" is the first sector to which writing will take place.
 * - "sectors_count" is the total number of sectors to be written.
 * - "buffer" is pre-allocated address in the memory from where the data will be
 *   loaded. each sector is loaded as (sector_size / 2) 16-bit integers; thus
 *   the required memory is sector_size * sectors_count. It is the
 *   responsibility of the caller to make sure that there's enough memory to
 *   store data.
 * - "callback" is a function that will be called once the write operation is
 *   completed. "callback_data" will be passed as an argument.
 *
 * Returns "true" when the request was accepted.
 */
bool ata_write(
    uint8_t ata_drive,
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
);

#endif
