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

static uint16_t** partition_buffer = NULL;
static uint8_t partition_buffer_used = 0;

static void _ata_identify_partitions_callback(void* data)
{
    uint8_t* drive_id_ptr = data;
    uint8_t drive_id = *drive_id_ptr;
    ata_mbr_t* mbr = (ata_mbr_t*)partition_buffer[drive_id - 1];

    _ata_load_partition_data(drive_id, mbr);

    kfree(partition_buffer[drive_id - 1]);
    partition_buffer_used--;
    if (partition_buffer_used == 0) {
        kfree(partition_buffer);
        partition_buffer = NULL;
    }
    kfree(drive_id_ptr);
}

static void _ata_identify_partitions(uint8_t drive_id)
{
    partition_buffer[drive_id - 1] = kmalloc_flags(
        sizeof(uint16_t) * 256,
        KMALLOC_ZERO
    );
    partition_buffer_used++;
    uint8_t* drive_id_ptr = kmalloc(sizeof(uint8_t));
    *drive_id_ptr = drive_id;
    ata_read(
        drive_id,
        0,
        1,
        partition_buffer[drive_id - 1],
        _ata_identify_partitions_callback,
        drive_id_ptr
    );
}

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

    partition_buffer = kmalloc_flags(sizeof(uint16_t*) * 4, KMALLOC_ZERO);
    uint8_t* avail_drives = kmalloc_flags(sizeof(uint8_t) * 5, KMALLOC_ZERO);
    ata_get_available_drives(avail_drives);
    uint8_t* avail_drives_ptr = avail_drives;
    while (*avail_drives_ptr) {
        _ata_identify_partitions(*avail_drives_ptr);
        avail_drives_ptr++;
    }
    kfree(avail_drives);
}
