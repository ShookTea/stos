#ifndef ARCH_I386_DRIVERS_ATA_ATAPI
#define ARCH_I386_DRIVERS_ATA_ATAPI

#include <stdint.h>

void _atapi_identify(uint16_t bus_base, uint8_t target_drive);

#endif
