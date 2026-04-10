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

/**
 * Handle IRQ event when current request is for writing a sector and the IRQ
 * confirms the end of entire writing process.
 */
static void irq_handler_flush()
{
    debug_puts("ATA IRQ for FLUSH - rescheduling");
    _ata_queue_schedule();
}

/**
 * Handle IRQ event when current request is for writing a sector and the IRQ
 * confirms that a single sector has been written.
 */
static void irq_handler_write(ds_ringbuf_t* queue, ata_request_t req)
{
    uint8_t drive = ata_get_selected_drive();
    bool primary = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_PRIMARY_SLAVE;
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Decrementing remaining sectors
    req.remaining_sectors--;

    debug_puts("ATA IRQ for WRITE start");

    if (req.remaining_sectors == 0) {
        // We finished writing - next IRQ will confirm that flush was completed.
        debug_puts("  last sector completed");
        req.awaiting_flush = true;

        // Send FLUSH command
        outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_FLUSH);
    } else {
        debug_printf("  %d sectors remaining\n", req.remaining_sectors);

        // Wait for drive to be ready to accept data (DRQ set, BSY clear)
        uint8_t status;
        do {
            status = _ata_read_status(bus_base);
        } while ((status & ATA_STATUS_BSY) || !(status & ATA_STATUS_DRQ));

        // Get current sector offset
        size_t sector_offset = req.total_sectors - req.remaining_sectors;
        // For write op: location on the buffer from where we should read data
        uint16_t* write_buff = req.buffer + (sector_offset * 256);
        // Sending bytes
        for (int i = 0; i < 256; i++) {
            outw(bus_base | ATA_BUS_OFFSET_DATA, write_buff[i]);
            io_wait();
        }
        // Acknowledge status
        _ata_read_status(bus_base);
    }

    ds_ringbuf_poke(queue, &req);
    debug_puts("ATA IRQ for WRITE completed");
}

/**
 * Handle IRQ event when current request is for reading a sector
 */
static void irq_handler_read(ds_ringbuf_t* queue, ata_request_t req)
{
    debug_puts("ATA IRQ for READ start");
    uint8_t drive = ata_get_selected_drive();
    bool primary = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_PRIMARY_SLAVE;
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // The current sector offset for given request
    size_t sector_offset = req.total_sectors - req.remaining_sectors;
    // For read op: location on the buffer to where we should read data
    uint16_t* read_buff = req.buffer + (sector_offset * 256);
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        read_buff[i] = data;
    }

    // Acknowledge status
    _ata_read_status(bus_base);

    // Reduce the number of remaining sectors
    req.remaining_sectors--;
    bool reschedule_required = req.remaining_sectors == 0;
    ds_ringbuf_poke(queue, &req);

    debug_printf(
        "ATA IRQ for READ %s\n",
        reschedule_required ? "completed with rescheduling" : "completed"
    );
    if (reschedule_required) {
        _ata_queue_schedule();
    }
}

static void _irq_handler()
{
    // Get current task, if any exist
    ds_ringbuf_t* queue = _ata_queue();
    ata_request_t req;
    bool req_loaded = false;
    if (queue != NULL) {
        req_loaded = ds_ringbuf_peek(queue, &req) == DS_SUCCESS;
    }

    if (!req_loaded) {
        return;
    }

    if (req.is_write && req.awaiting_flush) {
        // IRQ confirming end of writing
        irq_handler_flush();
    } else if (req.is_write) {
        // IRQ on write sector completed
        irq_handler_write(queue, req);
    } else {
        // IRQ on read sector completed
        irq_handler_read(queue, req);
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

void _test_write_callback(void* data __attribute__((unused)))
{
    debug_puts("Writing completed.");
    kfree(buffer);
}

void _test_read()
{
    buffer = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    ata_read(15, 2, buffer, _test_read_callback, NULL);
}

void _test_write()
{
    buffer = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    buffer[0] = 0xCAFE;
    buffer[1] = 0xBABE;
    buffer[256] = 0xDEAD;
    buffer[257] = 0xBEEF;
    buffer[4] = 0xCAFE;
    buffer[258] = 0xCAFE;
    ata_write(15, 2, buffer, _test_write_callback, NULL);
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
    // _test_write();
}
