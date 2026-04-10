#ifndef _LIBDS_RINGBUF
#define _LIBDS_RINGBUF

#include <stddef.h>
#include <stdbool.h>
#include <libds/libds.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type definition for ring buffer, for external use.
 */
typedef struct ds_ringbuf_s ds_ringbuf_t;

/**
 * Creates a new ring buffer, capable of fitting [capacity] number of items,
 * [item_size] bytes each. Caller is responsible for running ds_ringbuf_destroy
 * to deallocate the memory later.
 *
 * fail_on_full boolean determines the behavior when writing to a ring buffer
 * that is full:
 * - if set to "true", it will return an error status code (losing newest data)
 * - if set to "false", it will overwrite oldest value (losing oldest data)
 */
ds_ringbuf_t* ds_ringbuf_create(
    size_t capacity,
    size_t item_size,
    bool fail_on_full
);

/**
 * Destroys the ring buffer and deallocates the memory.
 */
void ds_ringbuf_destroy(ds_ringbuf_t*);

/**
 * Attempts to write value to a ringbuffer. The value is copied from given
 * pointer.
 *
 * Returns DS_SUCCESS on successful write, including when overwriting oldest
 * value (when fail_on_full = false). If fail_on_full = true and the ring buffer
 * is full, returns DS_FAIL_FULL instead.
 */
ds_result_t ds_ringbuf_push(ds_ringbuf_t* ringbuf, const void* value);

/**
 * Attempts to read value from a ringbuffer and copy it to "addr", then removes
 * that value from the buffer. Returns DS_SUCCESS on successful read, or
 * DS_ERR_EMPTY if the ringbuffer is empty.
 */
ds_result_t ds_ringbuf_pop(ds_ringbuf_t* ringbuf, void* addr);

/**
 * Attempts to read value from a ringbuffer and copy it to "addr". Returns
 * DS_SUCCESS on successful read, or DS_ERR_EMPTY if the ringbuffer is empty.
 */
ds_result_t ds_ringbuf_peek(const ds_ringbuf_t* ringbuf, void* addr);

/**
 * Overwrites the item at the current read pointer with the value copied from
 * "addr", without advancing the read pointer. Returns DS_SUCCESS on success,
 * DS_ERR_INVALID if ringbuf is NULL or DS_ERR_EMPTY if the ringbuffer is empty.
 */
ds_result_t ds_ringbuf_poke(ds_ringbuf_t* ringbuf, const void* addr);

/**
 * Clears content of the ringbuffer and resets read/write pointers.
 */
ds_result_t ds_ringbuf_clear(ds_ringbuf_t* ringbuf);

/**
 * Returns the current number of elements in the ring buffer.
 */
size_t ds_ringbuf_size(const ds_ringbuf_t* ringbuf);

/**
 * Returns the capacity (the max number of elements) in the ring buffer.
 */
size_t ds_ringbuf_capacity(const ds_ringbuf_t* ringbuf);

/**
 * Returns the remaining number of items that can be added to the ring buffer.
 */
inline size_t ds_ringbuf_free_space(const ds_ringbuf_t* ringbuf)
{
    return ds_ringbuf_capacity(ringbuf) - ds_ringbuf_size(ringbuf);
}

/**
 * Returns "true" if the ring buffer is empty.
 */
inline bool ds_ringbuf_is_empty(const ds_ringbuf_t* ringbuf)
{
    return ds_ringbuf_size(ringbuf) == 0;
}

/**
 * Returns "true" if the ring buffer is full.
 */
inline bool ds_ringbuf_is_full(const ds_ringbuf_t* ringbuf)
{
    return ds_ringbuf_size(ringbuf) == ds_ringbuf_capacity(ringbuf);
}

#ifdef __cplusplus
}
#endif

#endif
