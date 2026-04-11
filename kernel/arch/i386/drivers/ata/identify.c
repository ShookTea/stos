#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include "../../io.h"
#include "kernel/memory/kmalloc.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define _debug_puts(...) debug_puts_c("ATA", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA", __VA_ARGS__)

static uint32_t lba28_sec_count_primary_master = 0;
static uint32_t lba28_sec_count_primary_slave = 0;
static uint32_t lba28_sec_count_secondary_master = 0;
static uint32_t lba28_sec_count_secondary_slave = 0;

static uint8_t selected_drive = ATA_DRIVE_NONE;

static void _ata_identify(uint16_t bus_base, uint8_t target_drive)
{
    bool primary = bus_base == ATA_BUS_BASE_PRIMARY;
    bool master = target_drive == ATA_COM_TARGET_DRIVE_MASTER;

    _ata_drive_select(bus_base, target_drive);
    uint8_t status = _ata_read_status(bus_base);
    if (status == 0) {
        _debug_puts("Drive doesn't exist.");
        return;
    }

    // Clear values on ports
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_COUNT, 0);
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_NUMBER, 0);
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW, 0);
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH, 0);
    // Send IDENTIFY command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_IDENTIFY);

    // Read status
    status = _ata_read_status(bus_base);
    if (status == 0) {
        _debug_puts("Drive doesn't exist.");
        return;
    }

    // Wait for BSY flag to clear
    status = _ata_wait_for_bsy_clear(bus_base);

    // Check ATA_BUS_OFFSET_CYLINDER_LOW and _HIGH - they should be clear now
    if (inb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW) != 0) {
        _debug_puts("Invalid device: cylinder_low set");
        return;
    }

    if (inb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH) != 0) {
        _debug_puts("Invalid device: cylinder_high set");
        return;
    }

    // Continue polling until bit DRQ or ERR sets.
    while ((status & (ATA_STATUS_ERR | ATA_STATUS_DRQ)) == 0) {
        io_wait();
        status = _ata_read_status(bus_base);
        // TODO: introduce timeout here
    }

    if (status & ATA_STATUS_ERR) {
        _debug_puts("Invalid device: disk reports an error.");
        return;
    }

    // Data port now contains 256 16-bit values.
    uint32_t lba28_sectors_count = 0;
    uint64_t lba48_sectors_count = 0;
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        // if i=0 - it's apparently useful if device is not a hard disk.
        if (i == 88) {
            // Checking UDMA modes - low byte shows supported, high byte shows
            // the active mode.
            #if KERNEL_DEBUG_ANY
                uint8_t udma_sup = data & 0xFF;
                uint8_t udma_act = (data >> 8) & 0xFF;
                _debug_printf("UDMA sup=%#02x act=%#02x\n", udma_sup, udma_act);
            #endif
        } else if (i == 60) {
            // LBA28 sectors count
            lba28_sectors_count = data;
        } else if (i == 61) {
            lba28_sectors_count |= ((uint32_t)data << 16);
            if (lba28_sectors_count == 0) {
                _debug_puts("LBA28 not supported");
            } else {
                _debug_printf("LBA28 sectors count: %u\n", lba28_sectors_count);
                if (primary && master) {
                    lba28_sec_count_primary_master = lba28_sectors_count;
                } else if (primary && !master) {
                    lba28_sec_count_primary_slave = lba28_sectors_count;
                } else if (!primary && master) {
                    lba28_sec_count_secondary_master = lba28_sectors_count;
                } else if (!primary && !master) {
                    lba28_sec_count_secondary_slave = lba28_sectors_count;
                }
            }
        } else if (i == 100) {
            // LBA48 sectors count
            lba48_sectors_count = data;
        } else if (i == 101) {
            lba48_sectors_count |= (((uint64_t)data) << 16);
        } else if (i == 102) {
            lba48_sectors_count |= (((uint64_t)data) << 32);
        } else if (i == 103) {
            lba48_sectors_count |= (((uint64_t)data) << 48);
            if (lba48_sectors_count == 0) {
                _debug_puts("LBA48 not supported");
            } else {
                _debug_printf("LBA48 sec count: %llu\n", lba48_sectors_count);
            }
        }
    }

    _debug_printf("Drive found, status: %#x\n", status);
}

void _ata_identify_devices()
{
    _debug_puts("  Identifying master drive at primary port");
    _ata_identify(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_MASTER);
    _debug_puts("  Identifying slave drive at primary port");
    _ata_identify(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_SLAVE);
    _debug_puts("  Identifying master drive at secondary port");
    _ata_identify(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_MASTER);
    _debug_puts("  Identifying slave drive at secondary port");
    _ata_identify(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_SLAVE);

    // Selecting first available drive
    if (lba28_sec_count_primary_master > 0) {
        _ata_drive_select(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_MASTER);
        selected_drive = ATA_DRIVE_PRIMARY_MASTER;
        _debug_puts("Primary/master drive selected");
    } else if (lba28_sec_count_primary_slave > 0) {
        _ata_drive_select(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_SLAVE);
        selected_drive = ATA_DRIVE_PRIMARY_SLAVE;
        _debug_puts("Primary/slave drive selected");
    } else if (lba28_sec_count_secondary_master > 0) {
        _ata_drive_select(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_MASTER);
        selected_drive = ATA_DRIVE_SECONDARY_MASTER;
        _debug_puts("Secondary/master drive selected");
    } else if (lba28_sec_count_secondary_slave > 0) {
        _ata_drive_select(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_SLAVE);
        selected_drive = ATA_DRIVE_SECONDARY_SLAVE;
        _debug_puts("Secondary/slave drive selected");
    } else {
        _debug_puts("No driver selected");
    }
}

uint8_t ata_get_selected_drive()
{
    return selected_drive;
}

void ata_get_available_drives(uint8_t* res)
{
    memset(res, 0, sizeof(uint8_t) * 5);
    size_t idx = 0;
    if (lba28_sec_count_primary_master) {
        res[idx++] = ATA_DRIVE_PRIMARY_MASTER;
    }
    if (lba28_sec_count_primary_slave) {
        res[idx++] = ATA_DRIVE_PRIMARY_SLAVE;
    }
    if (lba28_sec_count_secondary_master) {
        res[idx++] = ATA_DRIVE_SECONDARY_MASTER;
    }
    if (lba28_sec_count_secondary_slave) {
        res[idx++] = ATA_DRIVE_SECONDARY_SLAVE;
    }
}

void ata_select_drive(uint8_t drive)
{
    selected_drive = drive;
    // TODO: implement drive switch?
}

uint32_t ata_lba28_sectors_count()
{
    switch (selected_drive) {
        case ATA_DRIVE_PRIMARY_MASTER:
            return lba28_sec_count_primary_master;
        case ATA_DRIVE_PRIMARY_SLAVE:
            return lba28_sec_count_primary_slave;
        case ATA_DRIVE_SECONDARY_MASTER:
            return lba28_sec_count_secondary_master;
        case ATA_DRIVE_SECONDARY_SLAVE:
            return lba28_sec_count_secondary_slave;
        default:
            return 0;
    }
}
