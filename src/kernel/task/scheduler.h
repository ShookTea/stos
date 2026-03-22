#ifndef KERNEL_TASK_SCHEDULER_H
#define KERNEL_TASK_SCHEDULER_H

#include "task.h"
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    /** Total number of tasks */
    size_t num_tasks;
    /** Tasks in TASK_WAITING state */
    size_t num_waiting;
    /** Tasks in TASK_BLOCKED state */
    size_t num_blocked;
    /** Tasks in TASK_SLEEPING state */
    size_t num_sleeping;
    /** Time spent in idle task */
    uint64_t total_idle_time;
    /** Currently running task */
    task_t* current_task;
} scheduler_stats_t;

/**
 * Initialize multitasking scheduler.
 */
void scheduler_init();

/**
 * Adds new task to the WAITING queue
 */
void scheduler_add_task(task_t* task);

/**
 * Remove task from all queues
 */
void scheduler_remove_task(task_t* task);

#endif
