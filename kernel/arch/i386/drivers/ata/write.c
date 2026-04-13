#include "./common.h"
#include "kernel/drivers/ata.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../io.h"

void _ata_write(ata_request_t* req)
{
    if (req == NULL || !req->is_write || req->atapi_phase != ATAPI_PHASE_NONE) {
        return;
    }

    // TODO: checking ATA status when reading needed
    bool primary = ata_drive_is_primary(req->drive);
    bool master = ata_drive_is_master(req->drive);
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
    // Send WRITE SECTORS command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_WRITE_SECTORS);

    // Wait for drive to be ready to accept data (DRQ set, BSY clear)
    uint8_t status;
    do {
        status = _ata_read_status(bus_base);
    } while ((status & ATA_STATUS_BSY) || !(status & ATA_STATUS_DRQ));

    // Sending only first sector - remaining sectors have to be written after
    // confirmation IRQ has been received
    ata_disk_info_t di;
    ata_load_disk_info(req->drive, &di);
    uint16_t sector_size_words = di.sector_size / 2;
    for (int i = 0; i < sector_size_words; i++) {
        outw(bus_base | ATA_BUS_OFFSET_DATA, req->buffer[i]);
        io_wait();
    }

    // Acknowledge status
    _ata_read_status(bus_base);
}
