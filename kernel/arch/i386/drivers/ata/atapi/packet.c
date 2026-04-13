#include "../common.h"
#include "./atapi.h"
#include "../../../io.h"
#include <stdint.h>
#include <string.h>

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
    // Clear DMA mode
    outb(bus_base | ATA_BUS_OFFSET_FEATURES, 0x00);

    // Send command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_ATAPI_PACKET);
}

void _atapi_send_command(
    uint8_t ata_drive,
    uint16_t byte_count,
    uint8_t atapi_packet[12],
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
) {
    ata_request_t req;
    req.drive = ata_drive;
    req.atapi_byte_count = byte_count;
    memcpy(req.atapi_packet, atapi_packet, sizeof(uint8_t) * 12);
    req.buffer = buffer;
    req.callback = callback;
    req.callback_data = callback_data;
    req.atapi_phase = ATAPI_PHASE_AWAITING_PACKET;
    req.total_sectors = 1;
    req.remaining_sectors = 1;
    req.awaiting_flush = false;
    req.is_write = false;
    _ata_enqueue_request(&req);
}
