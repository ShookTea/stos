#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../idt/pic.h"
#include "../../idt/idt.h"

static void _irq_handler()
{
    debug_puts("Received IRQ event on ATA");
}

void ata_init()
{
    debug_puts("Initializing ATA");
    _ata_identify_devices();
    uint8_t drive = ata_get_selected_drive();
    if (drive == ATA_DRIVE_NONE) {
        return;
    }
    if (drive == ATA_DRIVE_PRIMARY_MASTER || drive == ATA_DRIVE_PRIMARY_SLAVE) {
        idt_register_irq_handler(PIC_LINE_PRIMARY_ATA, &_irq_handler);
        pic_enable(PIC_LINE_PRIMARY_ATA);
    } {
        idt_register_irq_handler(PIC_LINE_SECONDARY_ATA, &_irq_handler);
        pic_enable(PIC_LINE_SECONDARY_ATA);
    }
    debug_puts("ATA IRQ enabled");
}
