#include <stdint.h>
#include "../../io.h"
#include "./common.h"
#include "../ata/common.h"
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
    // Data port now contains 256 16-bit values.
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        _debug_printf("[%03u] = 0x%04X\n", i, data);
    }

    _debug_puts("Status ready");
}
