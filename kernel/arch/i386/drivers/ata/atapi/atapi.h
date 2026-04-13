#ifndef ARCH_I386_DRIVERS_ATA_ATAPI
#define ARCH_I386_DRIVERS_ATA_ATAPI

#include <stdint.h>
#include "../common.h"

void _atapi_identify(uint16_t bus_base, uint8_t target_drive);
void _atapi_send_packet(ata_request_t* req);

#endif
