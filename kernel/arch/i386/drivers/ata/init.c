#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../idt/pic.h"
#include "../../idt/idt.h"
#include "../../io.h"
#include "kernel/memory/kmalloc.h"
#include <libds/libds.h>
#include <libds/ringbuf.h>

static void _irq_handler()
{
    uint8_t drive = ata_get_selected_drive();
    bool primary = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_PRIMARY_SLAVE;
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Get current task, if any exist
    ds_ringbuf_t* queue = _ata_queue();
    ata_request_t req;
    bool req_loaded = false;
    if (queue != NULL) {
        req_loaded = ds_ringbuf_peek(queue, &req) == DS_SUCCESS;
    }
    bool is_read = req_loaded && !req.is_write;
    // The current sector offset for given request
    size_t sector_offset = req_loaded
        ? (req.total_sectors - req.remaining_sectors)
        : 0;
    // For read op: location on the buffer to where we should read data
    uint16_t* read_buff = is_read ? (req.buffer + (sector_offset * 256)) : NULL;

    debug_puts("Received IRQ event on ATA");
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        if (read_buff != NULL) {
            read_buff[i] = data;
        }
    }

    // Reduce the number of remaining sectors
    bool reschedule_required = false;
    if (req_loaded) {
        req.remaining_sectors--;
        reschedule_required = req.remaining_sectors == 0;
        ds_ringbuf_poke(queue, &req);
    }

    // Read status register to acknowledge
    _ata_read_status(bus_base);
    debug_puts("Reading completed.");
    if (reschedule_required) {
        _ata_queue_schedule();
    }
}

static uint16_t* buffer = NULL;

void _test_read_callback(void* data __attribute__((unused)))
{
    debug_puts("Read completed, data:");
    for (size_t i = 0; i < 512; i++) {
        if (i % 16 == 0) {
            debug_puts("");
        }
        if (i == 256) {
            debug_puts("");
        }
        debug_printf("%04x ", buffer[i]);
    }
    debug_puts("");
    kfree(buffer);
}

void _test_read()
{
    buffer = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    ata_read(15, 2, buffer, _test_read_callback, NULL);
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
    } else {
        idt_register_irq_handler(PIC_LINE_SECONDARY_ATA, &_irq_handler);
        pic_enable(PIC_LINE_SECONDARY_ATA);
    }
    debug_puts("ATA IRQ enabled");

    _test_read();

    // uint16_t* data = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    // data[256] = 0xDEAD;
    // data[257] = 0xBEEF;
    // data[0] = 0xCAFE;
    // data[1] = 0xBABE;
    // _test_write(15, 2, data);
    // kfree(data);
}
