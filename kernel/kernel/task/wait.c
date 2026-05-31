#include "kernel/task/wait.h"
#include "kernel/drivers/pit.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include <stdbool.h>

static void enqueue_waiter(wait_obj_t* wait_obj, task_t* waiter)
{
    if (wait_obj->count == wait_obj->capacity) {
        wait_obj->capacity += 10;
        wait_obj->waiters = krealloc(
            wait_obj->waiters,
            sizeof(task_t*) * wait_obj->capacity
        );
    }
    wait_obj->waiters[wait_obj->count] = waiter;
    wait_obj->count++;
}

static void dequeue_waiter(wait_obj_t* wait_obj, task_t* waiter)
{
    for (size_t i = 0; i < wait_obj->count; i++) {
        if (wait_obj->waiters[i] != waiter) {
            continue;
        }

        // waiters[i] is a current task - let's move all remaining tasks in
        // a queue one step back
        for (size_t j = i + 1; j < wait_obj->count; j++) {
            wait_obj->waiters[j - 1] = wait_obj->waiters[j];
        }

        // Dequeueing completed.
        wait_obj->count--;
        break;
    }
}

static void wait_on_condition_impl(
    wait_obj_t* wait_obj,
    bool (*condition)(void* arg),
    void* arg,
    task_state_t block_state
) {
    task_t* task = scheduler_get_current_task();
    if (task == NULL) return;
    scheduler_move_task_to_state(task, block_state);
    enqueue_waiter(wait_obj, task);
    while (task->state == block_state && !condition(arg)) {
        scheduler_yield();
    }
    dequeue_waiter(wait_obj, task);
    // If condition was true before we ever yielded, the scheduler never had a
    // chance to move us back to RUNNING - we're still in blocked_queue with
    // block_state, which makes the next reschedule skip us permanently.
    if (task->state == block_state) {
        scheduler_move_task_to_state(task, TASK_RUNNING);
    }
}

void wait_on_condition(
    wait_obj_t* wait_obj,
    bool (*condition)(void* arg),
    void* arg
) {
    wait_on_condition_impl(wait_obj, condition, arg, TASK_BLOCKED);
}

void wait_wake_up(wait_obj_t* wait_obj)
{
    if (wait_obj->wakeup_all) {
        for (size_t i = 0; i < wait_obj->count; i++) {
            scheduler_move_task_to_state(wait_obj->waiters[i], TASK_WAITING);
        }
    } else if (wait_obj->count > 0) {
        scheduler_move_task_to_state(wait_obj->waiters[0], TASK_WAITING);
    }
}

static wait_obj_t* wait_alloc(bool wakeup_all)
{
    wait_obj_t* wait_obj = kmalloc_flags(sizeof(wait_obj_t), KMALLOC_ZERO);
    wait_obj->wakeup_all = wakeup_all;
    wait_obj->capacity = 5;
    wait_obj->waiters = kmalloc(sizeof(task_t*) * wait_obj->capacity);
    return wait_obj;
}

wait_obj_t* wait_allocate_queue()
{
    return wait_alloc(false);
}

wait_obj_t* wait_allocate_event()
{
    return wait_alloc(true);
}

void wait_deallocate(wait_obj_t* wait_obj)
{
    if (wait_obj == NULL) {
        return;
    }
    kfree(wait_obj->waiters);
    kfree(wait_obj);
}

typedef struct {
    bool done;
    wait_obj_t* wait_obj;
} wait_metadata_t;

static void pit_callback(void* _params)
{
    wait_metadata_t* params = _params;
    params->done = true;
    wait_wake_up(params->wait_obj);
}

static bool sleep_condition(void* _params)
{
    wait_metadata_t* params = _params;
    return params->done;
}

size_t wait_sleep(size_t millis)
{
    wait_obj_t* wait_obj = wait_allocate_queue();
    wait_metadata_t* meta = kmalloc(sizeof(wait_metadata_t));
    meta->done = false;
    meta->wait_obj = wait_obj;
    uint32_t start_tick = pit_get_tick();
    int timeout_id = pit_register_timeout(millis, pit_callback, meta);
    wait_on_condition_impl(wait_obj, sleep_condition, meta, TASK_SLEEPING);
    size_t remaining = 0;
    if (!meta->done) {
        pit_cancel_timeout(timeout_id);
        uint32_t elapsed = pit_get_tick() - start_tick;
        remaining = (elapsed < millis) ? (millis - elapsed) : 0;
    }
    wait_deallocate(wait_obj);
    kfree(meta);
    return remaining;
}
