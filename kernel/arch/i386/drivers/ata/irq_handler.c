#include "./common.h"
#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "../../io.h"
#include <libds/ringbuf.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../idt/idt.h"
#include "stdlib.h"

#define _debug_puts(...) debug_puts_c("ATA", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA", __VA_ARGS__)

/**
 * IRQ confirms that it received the PACKET command. Send the actual data to
 * the drive.
 */
static void irq_handler_atapi_awaiting_packet(
    ds_ringbuf_t* queue,
    ata_request_t req
) {
    _debug_puts("IRQ for ATAPI AWAITING PACKET");
    bool primary = ata_drive_is_primary(req.drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Send command
    for (size_t i = 0; i < 6; i++) {
        uint16_t w = req.atapi_packet[i * 2 + 1];
        w <<= 8;
        w |= req.atapi_packet[i * 2];
        outw(bus_base | ATA_BUS_OFFSET_DATA, w);
    }

    req.atapi_phase = ATAPI_PHASE_AWAITING_DATA;
    ds_ringbuf_poke(queue, &req);
    _debug_puts("ATAPI PACKET command sent");
}

/**
 * IRQ confirms that it received the data from PACKET command.
 */
static void irq_handler_atapi_awaiting_data(
    ds_ringbuf_t* queue,
    ata_request_t req
) {
    _debug_puts("IRQ for ATAPI AWAITING DATA");
    bool primary = ata_drive_is_primary(req.drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;
    uint8_t status = _ata_read_status(bus_base);

    // Read cylinder low/high for actual byte count
    uint16_t byte_low = inb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW);
    uint16_t byte_high = inb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH);
    if (status & ATA_STATUS_ERR) {
        // TODO: handle error
        _debug_printf("Error with codes 0x%02X 0x%02X\n", byte_high, byte_low);
        req.remaining_sectors = 0;
        ds_ringbuf_poke(queue, &req);
        _ata_queue_schedule(primary);
        abort();
        return;
    }

    uint16_t bytes_count = (byte_high << 8) | byte_low;
    uint16_t word_count = bytes_count / 2;

    _debug_printf("Words received: %u\n", word_count);
    for (uint16_t i = 0; i < word_count; i++) {
        req.buffer[i] = inw(bus_base | ATA_BUS_OFFSET_DATA);
    }

    // Set remaining_sectors = 0 to force reschedule
    req.remaining_sectors = 0;
    ds_ringbuf_poke(queue, &req);
    _ata_queue_schedule(primary);
}

/**
 * Handle IRQ event when current request is for writing a sector and the IRQ
 * confirms the end of entire writing process.
 */
static void irq_handler_flush(bool primary)
{
    _debug_puts("IRQ for FLUSH - rescheduling");
    _ata_queue_schedule(primary);
}

/**
 * Handle IRQ event when current request is for writing a sector and the IRQ
 * confirms that a single sector has been written.
 */
static void irq_handler_write(ds_ringbuf_t* queue, ata_request_t req)
{
    bool primary = ata_drive_is_primary(req.drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Decrementing remaining sectors
    req.remaining_sectors--;

    _debug_puts("IRQ for WRITE start");

    // Load sector size in words
    ata_disk_info_t di;
    ata_load_disk_info(req.drive, &di);
    uint16_t sector_size_words = di.sector_size / 2;

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
        uint16_t* write_buff = req.buffer + (sector_offset * sector_size_words);
        // Sending bytes
        for (int i = 0; i < sector_size_words; i++) {
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
    bool primary = ata_drive_is_primary(req.drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Load sector size in words
    ata_disk_info_t di;
    ata_load_disk_info(req.drive, &di);
    uint16_t sector_size_words = di.sector_size / 2;

    // The current sector offset for given request
    size_t sector_offset = req.total_sectors - req.remaining_sectors;
    // For read op: location on the buffer to where we should read data
    uint16_t* read_buff = req.buffer + (sector_offset * sector_size_words);
    for (int i = 0; i < sector_size_words; i++) {
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
        _ata_queue_schedule(primary);
    }
}

void _ata_irq_handler(registers_t* reg)
{
    bool primary = reg->int_no == 0x2E;
    // Get current task, if any exist
    ds_ringbuf_t* queue = _ata_queue(primary);
    ata_request_t req;
    bool req_loaded = false;
    if (queue != NULL) {
        req_loaded = ds_ringbuf_peek(queue, &req) == DS_SUCCESS;
    }

    if (!req_loaded) {
        return;
    }

    if (req.atapi_phase == ATAPI_PHASE_AWAITING_PACKET) {
        // IRQ confirming it received PACKET command
        irq_handler_atapi_awaiting_packet(queue, req);
    } else if (req.atapi_phase == ATAPI_PHASE_AWAITING_DATA) {
        // IRQ confirming it data after PACKET command
        irq_handler_atapi_awaiting_data(queue, req);
    } else if (req.is_write && req.awaiting_flush) {
        // IRQ confirming end of writing
        irq_handler_flush(primary);
    } else if (req.is_write) {
        // IRQ on write sector completed
        irq_handler_write(queue, req);
    } else {
        // IRQ on read sector completed
        irq_handler_read(queue, req);
    }
}
