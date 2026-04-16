#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "kernel/memory/kmalloc.h"
#include "kernel/spinlock.h"
#include <stdbool.h>
#include "./rw_queue.h"
#include "kernel/debug.h"
#include "stdlib.h"

#define _debug_puts(...) debug_puts_c("VFS/rwq", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/rwq", __VA_ARGS__)

size_t rwq_allocate_pos(rw_queue_t* queue)
{
    if (queue == NULL) {
        return 0;
    }

    spinlock_acquire(&queue->spinlock);
    if (queue->capacity == queue->count) {
        queue->capacity += 10;
        queue->rw_wait_map = (volatile rwq_status_t*)krealloc(
            (void*)queue->rw_wait_map,
            sizeof(rwq_status_t) * queue->capacity
        );
        memset((void*)(queue->rw_wait_map + queue->count), 0, 10);
        queue->rw_wait_map[queue->count] = RW_QUEUE_NOT_READY;
        queue->count++;
        spinlock_release(&queue->spinlock);
        return queue->count - 1;
    }

    for (size_t i = 0; i < queue->capacity; i++) {
        if (queue->rw_wait_map[i] == 0) {
            queue->rw_wait_map[i] = RW_QUEUE_NOT_READY;
            spinlock_release(&queue->spinlock);
            return i;
        }
    }
    _debug_puts("ERR: unexpected situation at allocate_rw_wait_pos");
    spinlock_release(&queue->spinlock);
    abort();
}

void rwq_deallocate_pos(rw_queue_t* queue, size_t idx)
{
    if (queue == NULL) {
        return;
    }

    spinlock_acquire(&queue->spinlock);
    if (idx >= queue->capacity) {
        _debug_puts("ERR: id >= capacity");
        spinlock_release(&queue->spinlock);
        return;
    }

    if (queue->rw_wait_map[idx] != RW_QUEUE_FREE_TO_USE) {
        queue->count--;
    }
    queue->rw_wait_map[idx] = RW_QUEUE_FREE_TO_USE;
    spinlock_release(&queue->spinlock);
}

bool rwq_set_ready(rw_queue_t* queue, size_t idx)
{
    if (queue == NULL) {
        return false;
    }

    spinlock_acquire(&queue->spinlock);
    if (idx >= queue->capacity) {
        _debug_puts("ERR: id >= capacity");
        spinlock_release(&queue->spinlock);
        return false;
    }

    if (queue->rw_wait_map[idx] == RW_QUEUE_FREE_TO_USE) {
        _debug_puts("ERR: id is free to use");
        spinlock_release(&queue->spinlock);
        return false;
    }
    queue->rw_wait_map[idx] = RW_QUEUE_READY;
    spinlock_release(&queue->spinlock);
    return true;
}

bool rwq_is_ready(rw_queue_t* queue, size_t idx)
{
    if (queue == NULL) {
        return false;
    }

    spinlock_acquire(&queue->spinlock);
    if (idx >= queue->capacity) {
        _debug_puts("ERR: id >= capacity");
        spinlock_release(&queue->spinlock);
        return false;
    }

    bool result = queue->rw_wait_map[idx] == RW_QUEUE_READY;
    spinlock_release(&queue->spinlock);
    return result;
}

void rwq_reset_to_not_ready(rw_queue_t* queue, size_t idx)
{
    if (queue == NULL) {
        return;
    }

    spinlock_acquire(&queue->spinlock);
    if (idx >= queue->capacity) {
        _debug_puts("ERR: id >= capacity");
        spinlock_release(&queue->spinlock);
        return;
    }

    if (queue->rw_wait_map[idx] == RW_QUEUE_FREE_TO_USE) {
        _debug_puts("ERR: id is free to use");
        spinlock_release(&queue->spinlock);
        return;
    }

    queue->rw_wait_map[idx] = RW_QUEUE_NOT_READY;
    spinlock_release(&queue->spinlock);
}
