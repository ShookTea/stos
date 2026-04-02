#include "kernel/task/wait.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include <stdbool.h>
#include <stdio.h>

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

void wait_on_condition(
    wait_obj_t* wait_obj,
    bool (*condition)(void* arg),
    void* arg
) {
    task_t* task = scheduler_get_current_task();

    if (task == NULL) {
        return;
    }
    printf("\n\tTask %u '%s' enters waiting on loop\n", task->pid, task->name);
    scheduler_move_task_to_state(task, TASK_BLOCKED);

    // Enqueue task in waiting object's queue
    enqueue_waiter(wait_obj, task);

    while (task->state == TASK_BLOCKED && !condition(arg)) {
        printf("\n\tTask %u '%s' still waiting\n", task->pid, task->name);
        scheduler_yield();
    }

    printf("\n\tTask %u '%s' finished waiting\n", task->pid, task->name);

    // Dequeueing the task
    dequeue_waiter(wait_obj, task);
    scheduler_move_task_to_state(task, TASK_WAITING);
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
