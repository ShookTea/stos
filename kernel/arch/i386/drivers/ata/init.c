#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../idt/pic.h"
#include "../../idt/idt.h"
#include "kernel/memory/kmalloc.h"
#include <libds/libds.h>
#include <libds/ringbuf.h>

#define _debug_puts(...) debug_puts_c("ATA", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA", __VA_ARGS__)

static uint16_t* buffer = NULL;

void _ata_load_mbr_callback(void* data __attribute__((unused)))
{
    _debug_puts("MBR load completed");
    ata_mbr_t* mbr = (ata_mbr_t*)buffer;
    _debug_printf("MBR signature bytes: %#04x\n", mbr->signature_bytes);
    _debug_printf("part1 type=%02x\n", mbr->partition_1.partition_type);
    _debug_printf("part2 type=%02x\n", mbr->partition_2.partition_type);
    _debug_printf("part3 type=%02x\n", mbr->partition_3.partition_type);
    _debug_printf("part4 type=%02x\n", mbr->partition_4.partition_type);
    kfree(buffer);
}

static void _ata_load_mbr()
{
    buffer = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    ata_read(0, 1, buffer, _ata_load_mbr_callback, NULL);
}

void ata_init()
{
    _debug_puts("Initializing ATA");
    _ata_identify_devices();
    uint8_t drive = ata_get_selected_drive();
    if (drive == ATA_DRIVE_NONE) {
        return;
    }
    if (drive == ATA_DRIVE_PRIMARY_MASTER || drive == ATA_DRIVE_PRIMARY_SLAVE) {
        idt_register_irq_handler(PIC_LINE_PRIMARY_ATA, &_ata_irq_handler);
        pic_enable(PIC_LINE_PRIMARY_ATA);
    } else {
        idt_register_irq_handler(PIC_LINE_SECONDARY_ATA, &_ata_irq_handler);
        pic_enable(PIC_LINE_SECONDARY_ATA);
    }
    _debug_puts("IRQ enabled");
    _ata_load_mbr();
}
