#ifndef KERNEL_TASK_WAIT_H
#define KERNEL_TASK_WAIT_H

#include "kernel/task/task.h"
#include <stdbool.h>

/**
 * Representation of a waiting object, that can be used for waking up tasks.
 */
typedef struct {
    task_t** waiters; // List of tasks waiting for this object
    size_t count; // Current number of waiting tasks
    size_t capacity; // Size of waiters pointer
    /**
     * "true" for event-like waiters, where we want to wake up all "listeners"
     * of the event; "false" for queue-like waiters (for example writing to a
     * file).
     */
    bool wakeup_all;
} task_waiter_obj_t;

/**
 * Block current task until a specific condition is passed. While task is
 * waiting, it stays in BLOCKED state and isn't scheduled.
 */
void wait_on_condition(bool (*condition)(void* arg), void* arg);

#endif
