#ifndef ARCH_I386_DRIVERS_ATA_ATAPI
#define ARCH_I386_DRIVERS_ATA_ATAPI

#include <stdint.h>
#include "../common.h"

#define ATAPI_COM_READ_CAPACITY 0x25

void _atapi_identify(uint16_t bus_base, uint8_t target_drive);

void _atapi_send_packet(ata_request_t* req);

void _atapi_send_command(
    uint8_t ata_drive,
    uint16_t byte_count,
    uint8_t atapi_packet[12],
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
);

#endif
