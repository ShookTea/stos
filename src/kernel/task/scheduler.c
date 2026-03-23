#include "scheduler.h"
#include <kernel/drivers/pit.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/tss.h>
#include <stdlib.h>
#include "task.h"

static uint8_t scheduler_tick_count = 0;
static scheduler_stats_t* scheduler_stats;
static task_t* waiting_queue = NULL;
static uint32_t idle_task_pid;

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

/**
 * This function returns next task that should be run, or NULL if we should
 * continue running the same task. It is responsible for updating queues if
 * neccessary.
 *
 * Current scheduling implemenation:
 * - if waiting queue doesn't have any tasks, that means that we're currently
 *   running the idle task (since idle task after running always goes back to
 *   that queue), but we don't have any other things to do now.
 *   We should continue running idle task now - returning NULL.
 * - if queue is non-empty, get the first item in the queue and:
 *   - if it's the idle task AND it's the only task, leave it there - we're
 *     running a single non-idle task and should continue doing so (ret. NULL)
 *   - if it's the idle task and there are other tasks in the queue, move idle
 *     task to the end of queue and take the next task instead for point below
 *   - if it's not the idle task, return it
 */
static task_t* scheduler_get_next_task()
{
    if (waiting_queue == NULL) {
        // We're currently running idle task, but there's nothing else to do,
        // so let's just keep it running.
        return NULL;
    }

    task_t* first_task_in_queue = waiting_queue;
    if (first_task_in_queue->pid != idle_task_pid) {
        // Not the idle task - remove it from the queue and return it
        waiting_queue = first_task_in_queue->next;
        first_task_in_queue->next = NULL;
        scheduler_stats->num_waiting--;
        return first_task_in_queue;
    }

    if (first_task_in_queue->next == NULL) {
        // This is the idle task, but there's nothing else - let's just keep
        // the current task running
        return NULL;
    }

    // The first task in the queue is the idle task, but there are some other
    // tasks waiting. First, let's move the idle task to the end of the queue.
    task_t* last = first_task_in_queue;
    while (last->next != NULL) {
        last = last->next;
    }
    last->next = first_task_in_queue;
    first_task_in_queue->prev = last;
    // Now set current queue to point to the task after the idle task
    waiting_queue = first_task_in_queue->next;
    first_task_in_queue->next = NULL;
    waiting_queue->prev = NULL;

    // We've rescheduled idle task to the end of the queue - now we should be
    // able to safely run scheduler_get_next_task() and it will return the new
    // top-priority task.
    return scheduler_get_next_task();
}

/**
 * Try to get next appropriate task from scheduler_get_next_task. If some task
 * is returned, we should switch to it.
 */
static void scheduler_reschedule()
{
    task_t* old_task = scheduler_stats->current_task;
    task_t* next_task = scheduler_get_next_task();
    if (next_task == NULL) {
        // No rescheduling needed
        return;
    }

    // Update task states
    old_task->state = TASK_WAITING;
    next_task->state = TASK_RUNNING;

    // Enqueue current task
    scheduler_add_task(old_task);
    // Switch current task to the next_task
    scheduler_stats->current_task = next_task;
    scheduler_switch_task_context(old_task, next_task);
}

static void scheduler_tick()
{
    // Re-add scheduler tick
    pit_register_timeout(SCHEDULER_TICK_TIME, scheduler_tick, NULL);
    // Increment scheduler tick count
    scheduler_tick_count++;

    // Increment task runtimes
    if (scheduler_stats->current_task->pid == idle_task_pid) {
        scheduler_stats->total_idle_time++;
    }
    scheduler_stats->current_task->total_runtime++;

    if (scheduler_tick_count == SCHEDULER_RESCHEDULE_COUNT) {
        scheduler_tick_count = 0;
        scheduler_reschedule();
    }
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

/**
 * Code for idle task - halts until next interrupt
 */
static void idle_task_function()
{
    while (1) {
         __asm__ volatile("hlt");
    }
}

void scheduler_init()
{
    scheduler_stats = kmalloc_flags(sizeof(scheduler_stats_t), KMALLOC_ZERO);

    // Create idle task - runs when nothing else is ready
    // TODO: when priorities are implemented, use lowest possible here
    task_t* idle_task = task_create("idle", idle_task_function, true);
    idle_task->state = TASK_RUNNING;
    idle_task_pid = idle_task->pid;
    scheduler_stats->current_task = idle_task;

    pit_register_timeout(SCHEDULER_TICK_TIME, scheduler_tick, NULL);
}

task_t* scheduler_get_current_task()
{
    if (scheduler_stats == NULL) {
        return NULL;
    }
    return scheduler_stats->current_task;
}
