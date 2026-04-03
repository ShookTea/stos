#include <libds/ringbuf.h>
#include <libds/libds.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct ds_ringbuf_s {
    size_t capacity;
    size_t item_size;
    size_t current_count;
    bool fail_on_full;
    size_t read_ptr;
    size_t write_ptr;
    void* storage;
};

static uint8_t* elem_ptr(const ds_ringbuf_t* ringbuff, size_t index)
{
    size_t byte_offset = ringbuff->item_size * index;
    return (uint8_t*)ringbuff->storage + byte_offset;
}

ds_ringbuf_t* ds_ringbuf_create(
    size_t capacity,
    size_t item_size,
    bool fail_on_full
) {
    ds_ringbuf_t* ringbuf = libds_malloc(sizeof(ds_ringbuf_t));

    ringbuf->capacity = capacity;
    ringbuf->item_size = item_size;
    ringbuf->fail_on_full = fail_on_full;
    ringbuf->read_ptr = 0;
    ringbuf->write_ptr = 0;
    ringbuf->current_count = 0;
    ringbuf->storage = libds_malloc(capacity * item_size);

    return ringbuf;
}

void ds_ringbuf_destroy(ds_ringbuf_t* ringbuff)
{
    if (ringbuff == NULL) {
        return;
    }
    libds_free(ringbuff->storage);
    libds_free(ringbuff);
}

ds_result_t ds_ringbuf_push(ds_ringbuf_t* ringbuf, const void* value)
{
    if (ringbuf == NULL) {
        return DS_ERR_INVALID;
    }

    if (ringbuf->current_count == ringbuf->capacity) {
        // Handle cases where ring buffer is full
        if (ringbuf->fail_on_full) {
            return DS_ERR_FULL;
        }

        // Overwriting - move reading pointer to the next entry (if we're full,
        // it points to the oldest entry)
        ringbuf->read_ptr = (ringbuf->read_ptr + 1) % ringbuf->capacity;
        ringbuf->current_count--; // Decrement for later incrementation
    }

    // Enter value to the ringbuf
    memcpy(
        elem_ptr(ringbuf, ringbuf->write_ptr),
        value,
        ringbuf->item_size
    );

    // Increment write pointer
    ringbuf->write_ptr = (ringbuf->write_ptr + 1) % ringbuf->capacity;
    ringbuf->current_count++;

    return DS_SUCCESS;
}

ds_result_t ds_ringbuf_pop(ds_ringbuf_t* ringbuf, void* addr)
{
    if (ringbuf == NULL) {
        return DS_ERR_INVALID;
    }

    if (ringbuf->current_count == 0) {
        return DS_ERR_EMPTY;
    }

    // Store value under the address
    memcpy(
        addr,
        elem_ptr(ringbuf, ringbuf->read_ptr),
        ringbuf->item_size
    );

    // Increment read pointer, decrement current size
    ringbuf->read_ptr = (ringbuf->read_ptr + 1) % ringbuf->capacity;
    ringbuf->current_count--;

    return DS_SUCCESS;
}

ds_result_t ds_ringbuf_peek(const ds_ringbuf_t* ringbuf, void* addr)
{
    if (ringbuf == NULL) {
        return DS_ERR_INVALID;
    }

    if (ringbuf->current_count == 0) {
        return DS_ERR_EMPTY;
    }

    // Store value under the address
    memcpy(
        addr,
        elem_ptr(ringbuf, ringbuf->read_ptr),
        ringbuf->item_size
    );

    return DS_SUCCESS;
}

ds_result_t ds_ringbuf_clear(ds_ringbuf_t* ringbuf)
{
    if (ringbuf == NULL) {
        return DS_ERR_INVALID;
    }
    ringbuf->current_count = 0;
    ringbuf->read_ptr = 0;
    ringbuf->write_ptr = 0;
    return DS_SUCCESS;
}

size_t ds_ringbuf_size(const ds_ringbuf_t* ringbuf)
{
    if (ringbuf == NULL) {
        return 0;
    }
    return ringbuf->current_count;
}

size_t ds_ringbuf_capacity(const ds_ringbuf_t* ringbuf)
{
    if (ringbuf == NULL) {
        return 0;
    }
    return ringbuf->capacity;
}
