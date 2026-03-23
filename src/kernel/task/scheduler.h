#ifndef KERNEL_TASK_SCHEDULER_H
#define KERNEL_TASK_SCHEDULER_H

#include "task.h"
#include <stdlib.h>
#include <stdint.h>

/**
 * Time between scheduler tick calls, in milliseconds. This is how often the
 * PIT controller will call scheduler_tick() function.
 */
#define SCHEDULER_TICK_TIME 10

/**
 * Number of scheduler ticks to happen since the last rescheduling to force
 * schedule change. The total max length of a single run of the task (in
 * milliseconds) is SCHEDULER_TICK_TIME * SCHEDULE_RESCHEDULE_COUNT.
 */
#define SCHEDULER_RESCHEDULE_COUNT 10

typedef struct {
    /** Total number of tasks */
    size_t num_tasks;
    /** Tasks in TASK_WAITING state */
    size_t num_waiting;
    /** Tasks in TASK_BLOCKED state */
    size_t num_blocked;
    /** Tasks in TASK_SLEEPING state */
    size_t num_sleeping;
    /** Time spent in idle task, in ticks */
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

/**
 * Returns the currently running task.
 */
task_t* scheduler_get_current_task();

#endif
