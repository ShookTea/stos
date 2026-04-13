#include "./common.h"
#include "atapi/atapi.h"
#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include <libds/libds.h>
#include <libds/ringbuf.h>
#include <string.h>

#define _debug_puts(...) debug_puts_c("ATA/queue", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA/queue", __VA_ARGS__)

#define QUEUE_SIZE 16

/**
 * Buffer containing all read/write requests, working as a queue.
 */
static ds_ringbuf_t* primary_queue = NULL;
static ds_ringbuf_t* secondary_queue = NULL;

/**
 * Set to true when there's a request currently being handled.
 */
static bool primary_req_in_progress = false;
static bool secondary_req_in_progress = false;

bool _ata_enqueue_request(ata_request_t* request)
{
    bool primary = ata_drive_is_primary(request->drive);
    if (primary && primary_queue == NULL) {
        primary_queue = ds_ringbuf_create(
            QUEUE_SIZE,
            sizeof(ata_request_t),
            true
        );
    }
    if (!primary && secondary_queue == NULL) {
        secondary_queue = ds_ringbuf_create(
            QUEUE_SIZE,
            sizeof(ata_request_t),
            true
        );
    }
    ds_ringbuf_t* queue = primary ? primary_queue : secondary_queue;

    bool result = ds_ringbuf_push(queue, request) == DS_SUCCESS;
    if (result) {
        _debug_printf(
            "%s task enqueued at %s\n",
            request->atapi_phase != ATAPI_PHASE_NONE
                ? "ATAPI" : request->is_write ? "write" : "read",
            primary ? "primary" : "secondary"
        );
        _ata_queue_schedule(primary);
    }
    return result;
}

void _ata_queue_schedule(bool primary)
{
    ds_ringbuf_t* queue = primary ? primary_queue : secondary_queue;
    if (queue == NULL || ds_ringbuf_is_empty(queue)) {
        _debug_printf(
            "no tasks at %s queue\n",
            primary ? "primary" : "secondary"
        );
        return;
    }
    ata_request_t req;
    ds_ringbuf_peek(queue, &req);

    if (primary ? primary_req_in_progress : secondary_req_in_progress) {
        // the current request is already in progress
        if (req.remaining_sectors == 0) {
            // ...but it has been completed. We should dequeue it and re-run
            // the schedule.
            _debug_puts("top task completed.");
            if (primary) {
                primary_req_in_progress = false;
            } else {
                secondary_req_in_progress = false;
            }
            req.callback(req.callback_data);
            ds_ringbuf_pop(queue, &req);
            _ata_queue_schedule(primary);
        } else {
            _debug_puts("current task still in progress");
        }

        // Either we called _ata_queue_schedule() again above, or the request
        // should still be in progress - nothing else to be done.
        return;
    }

    // Current request is a new request that should be handled appropriately.
    if (primary) {
        primary_req_in_progress = true;
    } else {
        secondary_req_in_progress = true;
    }
    if (req.atapi_phase == ATAPI_PHASE_AWAITING_PACKET) {
        _debug_puts("running ATAPI packet command");
        _atapi_send_packet(&req);
        // CDB was sent synchronously; advance phase so the next IRQ is treated
        // as a data transfer rather than a CDB handshake.
        req.atapi_phase = ATAPI_PHASE_AWAITING_DATA;
        ds_ringbuf_poke(queue, &req);
        _debug_puts("ATAPI packet command completed");
    } else if (req.is_write) {
        _debug_puts("running write command");
        _ata_write(&req);
        _debug_puts("write command completed");
    } else {
        _debug_puts("running read command");
        _ata_read(&req);
        _debug_puts("read command completed");
    }
}

static bool create_and_enqueue(
    uint8_t ata_drive,
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data,
    bool is_write
) {
    ata_disk_info_t info;
    if (!ata_load_disk_info(ata_drive, &info) || info.type == NOT_PRESENT) {
        return false;
    }

    ata_request_t req;
    req.drive = ata_drive;
    req.lba = lba;
    req.total_sectors = sectors_count;
    req.remaining_sectors = sectors_count;
    req.buffer = buffer;
    req.callback = callback;
    req.callback_data = callback_data;
    req.is_write = is_write;
    req.awaiting_flush = false;

    if (info.type == ATAPI) {
        memset(req.atapi_packet, 0, sizeof(req.atapi_packet));
        req.atapi_packet[0] = ATAPI_COM_READ10;
        req.atapi_packet[2] = (lba >> 24) & 0xFF;
        req.atapi_packet[3] = (lba >> 16) & 0xFF;
        req.atapi_packet[4] = (lba >> 8)  & 0xFF;
        req.atapi_packet[5] = lba         & 0xFF;
        req.atapi_packet[7] = 0;
        req.atapi_packet[8] = sectors_count;
        req.atapi_byte_count = info.sector_size;
        req.atapi_phase = ATAPI_PHASE_AWAITING_PACKET;
    } else {
        req.atapi_phase = ATAPI_PHASE_NONE;
    }

    return _ata_enqueue_request(&req);
}

bool ata_read(
    uint8_t ata_drive,
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
) {
    return create_and_enqueue(
        ata_drive,
        lba,
        sectors_count,
        buffer,
        callback,
        callback_data,
        false
    );
}

bool ata_write(
    uint8_t ata_drive,
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
) {
    return create_and_enqueue(
        ata_drive,
        lba,
        sectors_count,
        buffer,
        callback,
        callback_data,
        true
    );
}

ds_ringbuf_t* _ata_queue(bool primary)
{
    return primary ? primary_queue : secondary_queue;
}
