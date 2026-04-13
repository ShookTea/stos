#include <stdbool.h>
#include <stdint.h>
#include "../../../io.h"
#include "./atapi.h"
#include "../common.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c("ATAPI/id", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATAPI/id", __VA_ARGS__)

void _atapi_identify(uint16_t bus_base, uint8_t target_drive __attribute__((unused)))
{
    _debug_puts("Detecting ATAPI device");

    // bool primary = bus_base == ATA_BUS_BASE_PRIMARY;
    // bool master = target_drive == ATA_COM_TARGET_DRIVE_MASTER;

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
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        if (i == 0) {
            // word 0 contains some more details
            // size of packet - 16 bytes if true, 12 bytes if false
            bool packet_size_16 = (data & 0x0003) == 1;
            bool is_atapi = (data & 0xC000) == 0x8000;
            uint8_t dev_type = (data & 0x0700) >> 8;
            if (!is_atapi) {
                _debug_puts("Not ATAPI according to first bit");
                return;
            }
            _debug_printf("Packet size: %u B\n", packet_size_16 ? 16 : 12);
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
}
