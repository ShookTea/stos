#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../idt/pic.h"
#include "../../idt/idt.h"

#define _debug_puts(...) debug_puts_c("ATA", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA", __VA_ARGS__)

void ata_init()
{
    _debug_puts("Initializing ATA");
    _ata_identify_devices();
    uint8_t drive = ata_get_selected_drive();
    if (drive == ATA_DRIVE_NONE) {
        return;
    }
    idt_register_irq_handler(PIC_LINE_PRIMARY_ATA, &_ata_irq_handler);
    idt_register_irq_handler(PIC_LINE_SECONDARY_ATA, &_ata_irq_handler);
    pic_enable(PIC_LINE_PRIMARY_ATA);
    pic_enable(PIC_LINE_SECONDARY_ATA);
    _debug_puts("IRQ enabled");
}
