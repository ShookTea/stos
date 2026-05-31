#include <stdio.h>
#include <string.h>
#include "../proc.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/task.h"

static char get_state_character(task_t* task)
{
    switch (task->state) {
        case TASK_WAITING: return 'P';
        case TASK_RUNNING: return 'R';
        case TASK_BLOCKED: return 'D';
        case TASK_SLEEPING: return 'S';
        case TASK_ZOMBIE: return 'Z';
        case TASK_DEAD: return 'X';
        default: return '?';
    }
}

static char* build_buffer(task_t* task, size_t* total_written)
{
    size_t buff_len =
        10 // Realistically that should be enough for PID
        + 1 // Space
        + TASK_PROCESS_NAME_MAX_LEN + 2 // Max task name + parentheses
        + 1 // Space
        + 1 // State character
        + 1 // Space
        + 10 // Realistically that should be enough for parent PID
        + 1 // Space
        + 10 // Realistically that should be enough for process group ID
        + 1 // NULL at the end
    ;

    char* buff = kmalloc_flags(buff_len, KMALLOC_ZERO);

    size_t pos = 0;
    pos += sprintf(buff + pos, "%u (", task->pid); // PID + left parentheses
    pos += snprintf(buff + pos, TASK_PROCESS_NAME_MAX_LEN + 1, task->name);
    pos += sprintf(
        buff + pos,
        ") %c %u %u",
        get_state_character(task),
        task->parent == NULL ? 0 : task->parent->pid,
        task->pgid
    );

    *total_written = pos;
    return buff;
}

size_t proc_pid_read_stat(
    vfs_file_t* file,
    size_t offset,
    size_t size,
    void* ptr
) {
    proc_inode_meta_pid_t* meta = file->dentry->inode->metadata;
    task_t* task = task_get_task_by_pid(meta->pid);
    if (task == NULL) {
        return 0;
    }

    size_t buff_len;
    char* buff = build_buffer(task, &buff_len);

    if (offset >= buff_len) {
        kfree(buff);
        return 0;
    }

    size_t start_read = offset;
    size_t end_read = offset + size;
    if (end_read > buff_len) {
        end_read = buff_len;
    }
    size_t max_read = end_read - offset;

    memcpy(ptr, buff + start_read, max_read);
    kfree(buff);
    return max_read;
}
