#include "../common.h"
#include "./atapi.h"
#include "../../../io.h"

void _atapi_send_packet(ata_request_t* req)
{
    if (req == NULL || req->atapi_phase != ATAPI_PHASE_AWAITING_PACKET) {
        return;
    }

    // TODO: checking ATA status when reading needed
    uint8_t drive = req->drive;
    bool primary = ata_drive_is_primary(drive);
    bool master = ata_drive_is_master(drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Select drive
    outb(
        bus_base | ATA_BUS_OFFSET_DRIVE_HEAD,
        0xA0 | (master ? 0 : 1) << 4
    );

    // Write byte count
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW, req->atapi_byte_count & 0xFF);
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH, req->atapi_byte_count >> 8);

    // Send command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_ATAPI_PACKET);
}
