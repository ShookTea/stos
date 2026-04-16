#ifndef INCLUDE_KERNEL_SRC_VFS_RW_QUEUE_H
#define INCLUDE_KERNEL_SRC_VFS_RW_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "kernel/spinlock.h"

typedef enum {
    RW_QUEUE_FREE_TO_USE = 0,
    RW_QUEUE_NOT_READY = 1,
    RW_QUEUE_READY = 2
} rwq_status_t;

typedef struct {
    volatile rwq_status_t* rw_wait_map;
    size_t count;
    size_t capacity;
    spinlock_t spinlock;
} rw_queue_t;

#define RW_QUEUE_INIT { .rw_wait_map = NULL, .count = 0, .capacity = 0, .spinlock = SPINLOCK_INIT }

/**
 * Creates an entry in the queue, sets it to NOT_READY, and returns the ID.
 */
size_t rwq_allocate_pos(rw_queue_t*);

/**
 * Clears position, to be used by someone else
 */
void rwq_deallocate_pos(rw_queue_t*, size_t);

/**
 * Sets value under given index as READY.
 */
bool rwq_set_ready(rw_queue_t*, size_t);

/**
 * Returns true if value under given index is READY
 */
bool rwq_is_ready(rw_queue_t*, size_t);

/**
 * Sets value under given index back to NOT_READY
 */
void rwq_reset_to_not_ready(rw_queue_t*, size_t);

#endif
