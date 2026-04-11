#ifndef KERNEL_DRIVERS_ATA_H
#define KERNEL_DRIVERS_ATA_H

#include <stdint.h>
#include <stdbool.h>

#define ATA_DRIVE_NONE 0
#define ATA_DRIVE_PRIMARY_MASTER 1
#define ATA_DRIVE_PRIMARY_SLAVE 2
#define ATA_DRIVE_SECONDARY_MASTER 3
#define ATA_DRIVE_SECONDARY_SLAVE 4

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
uint8_t ata_get_selected_drive();

/**
 * Select a drive.
 */
void ata_select_drive(uint8_t);

/**
 * Returns sectors count of currently selected drive.
 */
uint32_t ata_lba28_sectors_count();

/**
 * Create a read request for ATA driver, for currently selected drive.
 * - "lba" is the first sector from which reading will take place.
 * - "sectors_count" is the total number of 512-byte sectors to be loaded.
 * - "buffer" is pre-allocated address in the memory where data will be stored.
 *   each sector is loaded as 256 16-bit integers; thus the required memory is
 *   256 * sizeof(uint16_t) * sectors_count. It is the responsibility of the
 *   caller to make sure that there's enough memory to store data.
 * - "callback" is a function that will be called once the read operation is
 *   completed. "callback_data" will be passed as an argument.
 *
 * Returns "true" when the request was accepted.
 */
bool ata_read(
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
);

/**
 * Create a write request for ATA driver, for currently selected drive.
 * - "lba" is the first sector to which writing will take place.
 * - "sectors_count" is the total number of 512-byte sectors to be written.
 * - "buffer" is pre-allocated address in the memory from where the data will be
 *   loaded. each sector is loaded as 256 16-bit integers; thus the required
 *   memory is 256 * sizeof(uint16_t) * sectors_count. It is the responsibility
 *   of the caller to make sure that there's enough memory to store data.
 * - "callback" is a function that will be called once the write operation is
 *   completed. "callback_data" will be passed as an argument.
 *
 * Returns "true" when the request was accepted.
 */
bool ata_write(
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
);

#endif
