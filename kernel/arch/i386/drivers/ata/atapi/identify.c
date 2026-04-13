#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../../../io.h"
#include "./atapi.h"
#include "../common.h"
#include "kernel/debug.h"
#include "kernel/drivers/ata.h"
#include "kernel/memory/kmalloc.h"

#define _debug_puts(...) debug_puts_c("ATAPI/id", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATAPI/id", __VA_ARGS__)

#define swipe_endian(w) ((uint16_t)((w >> 8) | (w << 8)))

typedef struct {
    uint8_t disk_id;
    uint8_t command;
} atapi_callback_t;

static uint16_t** callback_buffers;
static uint8_t callback_buffers_used = 0;

static void _callback(void* _data)
{
    atapi_callback_t* callback_data = _data;
    _debug_printf(
        "Callback received for DID=%u COM=0x%02X\n",
        callback_data->disk_id,
        callback_data->command
    );

    uint16_t* buf = callback_buffers[callback_data->disk_id];

    if (callback_data->command == ATAPI_COM_READ_CAPACITY) {
        // Last logical address block address
        uint16_t last_lba_high = swipe_endian(buf[0]);
        uint16_t last_lba_low = swipe_endian(buf[1]);
        uint32_t last_lba = ((uint32_t)last_lba_high) << 16;
        last_lba |= last_lba_low;

        // Size of single logical block in bytes
        uint16_t block_size_high = swipe_endian(buf[2]);
        uint16_t block_size_low = swipe_endian(buf[3]);
        uint32_t block_size = ((uint32_t)block_size_high) << 16;
        block_size |= block_size_low;

        _debug_printf(
            "Last LBA=%lu, block size = %lu B, total size = %llu B\n",
            last_lba,
            block_size,
            ((uint64_t)last_lba) * block_size
        );
    }

    kfree(callback_buffers[callback_data->disk_id]);
    callback_buffers_used--;
    if (callback_buffers_used == 0) {
        kfree(callback_buffers);
    }
    kfree(callback_data);
}

void _atapi_identify(uint16_t bus_base, uint8_t target_drive)
{
    _debug_puts("Detecting ATAPI device");
    bool primary = bus_base == ATA_BUS_BASE_PRIMARY;
    bool master = target_drive == ATA_COM_TARGET_DRIVE_MASTER;
    uint8_t disk_id = (primary ? 0b00 : 0b10) | (master ? 0b00 : 0b01);

    // Send identify packet device command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_IDENTIFY_PACKET);

    // Wait for data
    _ata_wait_for_bsy_clear(bus_base);
    uint8_t status = _ata_read_status(bus_base);
    while ((status & (ATA_STATUS_ERR | ATA_STATUS_DRQ)) == 0) {
        io_wait();
        status = _ata_read_status(bus_base);
        // TODO: introduce timeout here
    }
    if (status & ATA_STATUS_ERR) {
        _debug_puts("ATAPI identify error");
        return;
    }

    // Data port now contains 256 16-bit values.
    char firmware_name[40];
    uint8_t packet_size = 0;
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        if (i == 0) {
            // word 0 contains some more details
            // size of packet - 16 bytes if true, 12 bytes if false
            packet_size = ((data & 0x0003) == 1) ? 16 : 12;
            bool is_atapi = (data & 0xC000) == 0x8000;
            uint8_t dev_type = (data & 0x0700) >> 8;
            if (!is_atapi) {
                _debug_puts("Not ATAPI according to first bit");
                return;
            }
            _debug_printf("Packet size: %u B\n", packet_size);
            switch (dev_type) {
                case 0x05:
                    _debug_puts("Device type: CD-ROM");
                    break;
                default:
                    _debug_printf("Unrecognized device type: %x\n", dev_type);
                    return;
            }
        }
        else if (i >= 27 && i <= 46) {
            firmware_name[(i - 27) * 2] = (data >> 8) & 0xFF;
            firmware_name[(i - 27) * 2 + 1] = data & 0xFF;
            if (i == 46) {
                // Set last char to NULL
                firmware_name[39] = 0;
                // Iterate from back, clearing all whitespaces
                size_t i = 38;
                while (firmware_name[i] == ' ') {
                    firmware_name[i] = 0;
                    i--;
                }
                _debug_printf("Firmware: '%s'\n", firmware_name);
            }
        }
    }

    ata_disk_info_t di;
    di.type = ATAPI;
    di.sector_size = 0;
    di.sectors_count = 0;

    strcpy(di.firmare_name, firmware_name);
    _ata_save_disk_info(disk_id, &di);

    _debug_puts("Drive found - sending READ CAPACITY command");

    if (callback_buffers == NULL) {
        callback_buffers = kmalloc_flags(sizeof(uint16_t*) * 4, KMALLOC_ZERO);
    }
    callback_buffers[disk_id] = kmalloc(sizeof(uint16_t) * 4); // 8 bytes return
    callback_buffers_used++;

    atapi_callback_t* data = kmalloc(sizeof(atapi_callback_t));
    data->disk_id = disk_id;
    data->command = ATAPI_COM_READ_CAPACITY;
    uint8_t packet[12] = { ATAPI_COM_READ_CAPACITY };
    _atapi_send_command(
        disk_id,
        8,
        packet,
        callback_buffers[disk_id],
        _callback,
        data
    );
}
