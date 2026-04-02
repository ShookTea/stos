#include "kernel/task/wait.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include <stdbool.h>
#include <stdio.h>

void wait_on_condition(bool (*condition)(void* arg), void* arg)
{
    task_t* task = scheduler_get_current_task();

    if (task == NULL) {
        return;
    }
    printf("\n\tTask %u '%s' enters waiting on loop\n", task->pid, task->name);

    scheduler_move_task_to_state(task, TASK_BLOCKED);
    while (!condition(arg)) {
        printf("\n\tTask %u '%s' still waiting\n", task->pid, task->name);
        scheduler_yield();
    }

    printf("\n\tTask %u '%s' finished waiting\n", task->pid, task->name);

    scheduler_move_task_to_state(task, TASK_WAITING);
}
