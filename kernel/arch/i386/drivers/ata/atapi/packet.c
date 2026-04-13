#include "../common.h"
#include "./atapi.h"
#include "../../../io.h"
#include "kernel/debug.h"
#include <stdint.h>
#include <string.h>

#define _debug_puts(...) debug_puts_c("ATAPI/pkt", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATAPI/pkt", __VA_ARGS__)

void _atapi_send_packet(ata_request_t* req)
{
    if (req == NULL || req->atapi_phase != ATAPI_PHASE_AWAITING_PACKET) {
        return;
    }

    bool primary = ata_drive_is_primary(req->drive);
    bool master = ata_drive_is_master(req->drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    _debug_printf(
        "drive=%u (%s/%s) byte_count=%u status=0x%02X\n",
        req->drive,
        primary ? "primary" : "secondary",
        master ? "master" : "slave",
        req->atapi_byte_count,
        _ata_read_status(bus_base)
    );

    // Select drive
    outb(bus_base | ATA_BUS_OFFSET_DRIVE_HEAD, 0xA0 | (master ? 0 : 1) << 4);
    // Write byte count limit per DRQ transfer
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW, req->atapi_byte_count & 0xFF);
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH, req->atapi_byte_count >> 8);
    // PIO mode
    outb(bus_base | ATA_BUS_OFFSET_FEATURES, 0x00);
    // Send PACKET command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_ATAPI_PACKET);

    // Poll for BSY to clear and DRQ to set (drive ready for CDB).
    // We do this synchronously instead of waiting for an IRQ: QEMU responds
    // so fast that the data-ready IRQ fires before the CDB-ready IRQ is even
    // serviced, causing the second rising edge to be lost on the edge-triggered
    // 8259 PIC while IRQ15 is still in-service from the first handler.
    uint8_t status = _ata_wait_for_bsy_clear(bus_base);
    if (status & ATA_STATUS_ERR) {
        _debug_printf("Error waiting for DRQ: status=0x%02X\n", status);
        return;
    }
    while (!(status & ATA_STATUS_DRQ)) {
        status = _ata_read_status(bus_base);
    }

    // Send 12-byte CDB as 6 16-bit words
    _debug_printf(
        "CDB: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
        req->atapi_packet[0],  req->atapi_packet[1],
        req->atapi_packet[2],  req->atapi_packet[3],
        req->atapi_packet[4],  req->atapi_packet[5],
        req->atapi_packet[6],  req->atapi_packet[7],
        req->atapi_packet[8],  req->atapi_packet[9],
        req->atapi_packet[10], req->atapi_packet[11]
    );
    for (size_t i = 0; i < 6; i++) {
        uint16_t w = req->atapi_packet[i * 2 + 1];
        w <<= 8;
        w |= req->atapi_packet[i * 2];
        outw(bus_base | ATA_BUS_OFFSET_DATA, w);
    }
    _debug_puts("CDB sent, waiting for data IRQ");
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
