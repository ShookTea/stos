#include "./common.h"
#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "../../io.h"
#include <libds/ringbuf.h>
#include <stdint.h>
#include <stdbool.h>

#define _debug_puts(...) debug_puts_c("ATA", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA", __VA_ARGS__)

/**
 * Handle IRQ event when current request is for writing a sector and the IRQ
 * confirms the end of entire writing process.
 */
static void irq_handler_flush()
{
    _debug_puts("IRQ for FLUSH - rescheduling");
    _ata_queue_schedule();
}

/**
 * Handle IRQ event when current request is for writing a sector and the IRQ
 * confirms that a single sector has been written.
 */
static void irq_handler_write(ds_ringbuf_t* queue, ata_request_t req)
{
    uint8_t drive = ata_get_selected_drive();
    bool primary = ata_drive_is_primary(drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Decrementing remaining sectors
    req.remaining_sectors--;

    _debug_puts("IRQ for WRITE start");

    if (req.remaining_sectors == 0) {
        // We finished writing - next IRQ will confirm that flush was completed.
        _debug_puts("  last sector completed");
        req.awaiting_flush = true;

        // Send FLUSH command
        outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_FLUSH);
    } else {
        _debug_printf("  %d sectors remaining\n", req.remaining_sectors);

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
    _debug_puts("IRQ for WRITE completed");
}

/**
 * Handle IRQ event when current request is for reading a sector
 */
static void irq_handler_read(ds_ringbuf_t* queue, ata_request_t req)
{
    _debug_puts("IRQ for READ start");
    uint8_t drive = ata_get_selected_drive();
    bool primary = ata_drive_is_primary(drive);
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

    _debug_printf(
        "IRQ for READ %s\n",
        reschedule_required ? "completed with rescheduling" : "completed"
    );
    if (reschedule_required) {
        _ata_queue_schedule();
    }
}

void _ata_irq_handler()
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
