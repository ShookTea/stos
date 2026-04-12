#ifndef ARCH_I386_DRIVERS_ATAPI_COMMON
#define ARCH_I386_DRIVERS_ATAPI_COMMON

#include <stdint.h>

void _atapi_identify(uint16_t bus_base, uint8_t target_drive);

#endif
