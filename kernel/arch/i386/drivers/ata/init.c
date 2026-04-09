#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include "../../io.h"
#include <stdint.h>
#include <stdbool.h>

static void _ata_identify(uint16_t bus_base, uint8_t target_drive)
{
    _ata_drive_select(bus_base, target_drive);
    uint8_t status = _ata_read_status(bus_base);
    if (status == 0) {
        debug_puts("Drive doesn't exist.");
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
        debug_puts("Drive doesn't exist.");
        return;
    }

    // Wait for BSY flag to clear
    status = _ata_wait_for_bsy_clear(bus_base);

    // Check ATA_BUS_OFFSET_CYLINDER_LOW and _HIGH - they should be clear now
    if (inb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW) != 0) {
        debug_puts("Invalid device: cylinder_low set");
        return;
    }

    if (inb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH) != 0) {
        debug_puts("Invalid device: cylinder_high set");
        return;
    }

    // Continue polling until bit DRQ or ERR sets.
    while ((status & (ATA_STATUS_ERR | ATA_STATUS_DRQ)) == 0) {
        io_wait();
        status = _ata_read_status(bus_base);
        // TODO: introduce timeout here
    }

    if (status & ATA_STATUS_ERR) {
        debug_puts("Invalid device: disk reports an error.");
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
            uint8_t udma_sup = data & 0xFF;
            uint8_t udma_act = (data >> 8) & 0xFF;
            debug_printf("UDMA sup=%#02x act=%#02x\n", udma_sup, udma_act);
        } else if (i == 60) {
            // LBA28 sectors count
            lba28_sectors_count = data;
        } else if (i == 61) {
            lba28_sectors_count |= (data << 16);
            if (lba28_sectors_count == 0) {
                debug_puts("LBA28 not supported");
            } else {
                debug_printf("LBA28 sectors count: %u\n", lba28_sectors_count);
            }
        } else if (i == 100) {
            // LBA48 sectors count
            lba28_sectors_count = data;
        } else if (i == 101) {
            lba28_sectors_count |= (((uint64_t)data) << 16);
        } else if (i == 102) {
            lba28_sectors_count |= (((uint64_t)data) << 32);
        } else if (i == 103) {
            lba28_sectors_count |= (((uint64_t)data) << 48);
            if (lba48_sectors_count == 0) {
                debug_puts("LBA48 not supported");
            } else {
                debug_printf("LBA48 sectors count: %u\n", lba48_sectors_count);
            }
        }
    }

    debug_printf("Drive found, status: %#x\n", status);
}

void ata_init()
{
    debug_puts("Initializing ATA");
    debug_puts("  Identifying master drive at primary port");
    _ata_identify(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_MASTER);
    debug_puts("  Identifying slave drive at primary port");
    _ata_identify(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_MASTER);
    debug_puts("  Identifying master drive at secondary port");
    _ata_identify(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_SLAVE);
    debug_puts("  Identifying slave drive at secondary port");
    _ata_identify(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_SLAVE);
}
