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
} wait_obj_t;

/**
 * Block current task until a specific condition is passed. While task is
 * waiting, it stays in BLOCKED state and isn't scheduled.
 */
void wait_on_condition(
    wait_obj_t* wait_obj,
    bool (*condition)(void* arg),
    void* arg
);

/**
 * Wakes up first task in the queue or all of them, depending on the value of
 * wakeup_all. They will re-check their condition and continue the execution
 * if condition is passed.
 */
void wait_wake_up(wait_obj_t* wait_obj);

/**
 * Allocates a memory for wait_obj that works as a queue (wakeup_all=false).
 * It is the responsibility of the caller to call void wait_deallocate() when
 * it's no longer needed.
 */
wait_obj_t* wait_allocate_queue();

/**
 * Allocates a memory for wait_obj that works as an event listener
 * (wakeup_all=true).
 * It is the responsibility of the caller to call void wait_deallocate() when
 * it's no longer needed.
 */
wait_obj_t* wait_allocate_event();

/**
 * Frees the memory for given wait_obj.
 */
void wait_deallocate(wait_obj_t* wait_obj);

#endif
