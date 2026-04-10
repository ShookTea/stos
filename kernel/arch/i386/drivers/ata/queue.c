#include "./common.h"
#include "kernel/drivers/ata.h"
#include <libds/libds.h>
#include <libds/ringbuf.h>

#define QUEUE_SIZE 16

/**
 * Buffer containing all read/write requests, working as a queue.
 */
static ds_ringbuf_t* queue = NULL;

/**
 * Set to true when there's a request currently being handled.
 */
static bool req_in_progress = false;

bool _ata_enqueue_request(ata_request_t* request)
{
    if (queue == NULL) {
        ds_ringbuf_create(
            QUEUE_SIZE,
            sizeof(ata_request_t),
            true
        );
    }

    bool result = ds_ringbuf_push(queue, request) == DS_SUCCESS;
    if (result) {
        _ata_queue_schedule();
    }
    return result;
}

void _ata_queue_schedule()
{
    if (queue == NULL || req_in_progress || ds_ringbuf_is_empty(queue)) {
        return;
    }
    req_in_progress = true;

    ata_request_t req;
    ds_ringbuf_peek(queue, &req);
}

static bool create_and_enqueue(
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data,
    bool is_write
) {
    ata_request_t req;
    req.lba = lba;
    req.total_sectors = sectors_count;
    req.remaining_sectors = sectors_count;
    req.buffer = buffer;
    req.callback = callback;
    req.callback_data = callback_data;
    req.is_write = is_write;

    return _ata_enqueue_request(&req);
}

bool ata_read(
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
) {
    return create_and_enqueue(
        lba,
        sectors_count,
        buffer,
        callback,
        callback_data,
        false
    );
}

bool ata_write(
    uint32_t lba,
    uint8_t sectors_count,
    uint16_t* buffer,
    void (*callback)(void*),
    void* callback_data
) {
    return create_and_enqueue(
        lba,
        sectors_count,
        buffer,
        callback,
        callback_data,
        true
    );
}

ds_ringbuf_t* _ata_queue()
{
    return queue;
}
