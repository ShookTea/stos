#include "./common.h"
#include "kernel/drivers/ata.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../io.h"

void _ata_read(ata_request_t* req)
{
    if (req == NULL || req->is_write || req->atapi_phase != ATAPI_PHASE_NONE) {
        return;
    }

    // TODO: checking ATA status when reading needed
    uint8_t drive = ata_get_selected_drive();
    bool primary = ata_drive_is_primary(drive);
    bool master = ata_drive_is_master(drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Select drive + top 4 bits of LBA
    outb(
        bus_base | ATA_BUS_OFFSET_DRIVE_HEAD,
        0xE0 | ((master ? 0 : 1) << 4) | ((req->lba >> 24) & 0x0F)
    );
    // Send sector count + LBA bits 0-23
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_COUNT, req->total_sectors);
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_NUMBER, (uint8_t)(req->lba));
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW, (uint8_t)(req->lba >> 8));
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH, (uint8_t)(req->lba >> 16));
    // Send READ SECTORS command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_READ_SECTORS);

    // Acknowledge status
    _ata_read_status(bus_base);

    // Now we'll one IRQ for each sector_count. Each such interrupt will give us
    // 256 16-bit values on port bus_base | ATA_BUS_OFFSET_DATA.
}
