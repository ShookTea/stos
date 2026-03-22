#include "scheduler.h"
#include "kernel/drivers/pit.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/tss.h"
#include "stdlib.h"
#include "task.h"

static uint8_t scheduler_tick_count = 0;
static scheduler_stats_t* scheduler_stats;
static task_t* waiting_queue = NULL;

extern void switch_to_stack(
    uint32_t* old_esp_ptr,
    uint32_t new_esp,
    uint32_t new_cr3
);

/**
 * Switch context from one task to another
 */
static void scheduler_switch_task_context(
    task_t* old_task,
    task_t* new_task
) {
    if (new_task == NULL) {
        return; // Nothing to switch to
    }
    // Update TSS with new kernel stack - needs to be done before switching
    tss_set_kernel_stack(
        new_task->kernel_stack_base,
        new_task->kernel_stack_size
    );

    // Get pointers for context switch
    uint32_t* old_esp_ptr = (old_task != NULL) ? &old_task->context.esp : NULL;
    uint32_t new_esp = new_task->context.esp;
    uint32_t new_cr3 = new_task->page_dir_phys;

    // Perform the actual switch
    switch_to_stack(old_esp_ptr, new_esp, new_cr3);

    // TODO: when we return here, we're running as new_task now.
    // Should we do anything?
}

static void scheduler_tick()
{
    // Increment scheduler tick count
    scheduler_tick_count++;
    if (scheduler_stats->current_task == NULL) {
        scheduler_stats->total_idle_time++;
    }

    if (scheduler_tick_count == SCHEDULER_RESCHEDULE_COUNT) {
        scheduler_tick_count = 0;
        // TODO: run rescheduling here
    }

    // Re-add scheduler tick
    pit_register_timeout(SCHEDULER_TICK_TIME, scheduler_tick, NULL);
}

void scheduler_add_task(task_t* task)
{
    if (waiting_queue == NULL) {
        waiting_queue = task;
    } else {
        task_t* last = waiting_queue;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = task;
        task->prev = last;
    }
    scheduler_stats->num_tasks++;
    scheduler_stats->num_waiting++;
}

void scheduler_remove_task(task_t* task)
{
    task_t* queue = NULL;
    switch (task->state) {
        case TASK_WAITING: queue = waiting_queue; break;
        default: break;
        // TODO: handle other queues
    }

    if (queue == NULL) {
        // That should never happen - the task is in some unrecognized state
        abort();
    }

    while (queue->pid != task->pid && queue->next != NULL) {
        queue = queue->next;
    }

    if (queue == NULL) {
        // That should never happen - the task is not in the queue
        abort();
    }

    // Remove task from the queue
    if (queue->prev != NULL) {
        queue->prev->next = queue->next;
    }
    if (queue->next != NULL) {
        queue->next->prev = queue->prev;
    }
}

void scheduler_init()
{
    pit_register_timeout(SCHEDULER_TICK_TIME, scheduler_tick, NULL);
    scheduler_stats = kmalloc_flags(sizeof(scheduler_stats_t), KMALLOC_ZERO);
}
