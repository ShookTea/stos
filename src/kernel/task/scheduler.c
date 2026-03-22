#include "scheduler.h"
#include "kernel/drivers/pit.h"

// Time between scheduler tick calls, in milliseconds
// Thi
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

static uint8_t scheduler_tick_count = 0;

static void scheduler_tick()
{
    // Increment scheduler tick count
    scheduler_tick_count++;

    if (scheduler_tick_count == SCHEDULER_RESCHEDULE_COUNT) {
        scheduler_tick_count = 0;
        // TODO: run rescheduling here
    }

    // Re-add scheduler tick
    pit_register_timeout(SCHEDULER_TICK_TIME, scheduler_tick, NULL);
}

void scheduler_add_task(task_t* task);

void scheduler_remove_task(task_t* task);

void scheduler_init()
{
    pit_register_timeout(SCHEDULER_TICK_TIME, scheduler_tick, NULL);
}
