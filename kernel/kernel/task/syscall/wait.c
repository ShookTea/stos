#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "../syscall.h"
#include <stdbool.h>
#include <stdio.h>

static task_t* find_child(task_t* parent, int pid)
{
    task_t* child = parent->first_child;
    while (child != NULL && child->pid != (uint32_t)pid) {
        child = child->next_sibling;
    }
    if (child != NULL && child->state == TASK_ZOMBIE) {
        return child;
    }
    return NULL;
}

static task_t* find_any_zombie_child(task_t* parent)
{
    task_t* child = parent->first_child;
    while (child != NULL && child->state != TASK_ZOMBIE) {
        child = child->next_sibling;
    }
    return child;
}

int sys_wait(int pid, int* status_code, int options)
{
    task_t* curr = scheduler_get_current_task();
    if (curr == NULL) {
        return SYSCALL_ERROR;
    }

    if (options & SYS_WAIT_OPT_NO_HANG) {
        // If this option is true, we don't want to hang when no child has
        // exited. We should first check if the child exists and is in ZOMBIE
        // state.
        if (pid > 0) {
            task_t* child = find_child(curr, pid);
            if (child == NULL) {
                return SYSCALL_ERROR; // TODO: set error code
            } else if (child->state != TASK_ZOMBIE) {
                return 0; // Special case, when PID exists but is not zombie
            }
            // In other case, child is found and is a zombie - run a standard
            // code for waiting and reaping
        } else {
            // PID=0 passed - does the task have any children at all?
            if (curr->first_child == NULL) {
                return SYSCALL_ERROR; // TODO: set error code
            }
            // Check if there is any zombie child
            task_t* zombie = find_any_zombie_child(curr);
            if (zombie == NULL) {
                return 0; // Child exists, but is not a zombie
            }
            // In other case, we can run a standard code for reaping
        }
    }
    int reaped_pid = task_wait(pid, status_code);
    if (reaped_pid > 0) {
        return reaped_pid;
    }

    return SYSCALL_ERROR; // TODO: analyze return value and set error code
}
