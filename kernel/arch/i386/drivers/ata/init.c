#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include <stdint.h>
#include <stdbool.h>

void ata_init()
{
    debug_puts("Initializing ATA");
    _ata_identify_devices();
}
