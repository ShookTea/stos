#ifndef KERNEL_DRIVERS_ATA_H
#define KERNEL_DRIVERS_ATA_H

#include <stdint.h>

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
 * Returns one of ATA_DRIVE_ values.
 */
uint8_t ata_get_selected_driver();

/**
 * Returns sectors count of currently selected drive.
 */
uint32_t ata_lba28_sectors_count();

#endif
