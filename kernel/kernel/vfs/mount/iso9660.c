#include "kernel/memory/kmalloc.h"
#include "kernel/spinlock.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "./mount.h"
#include "stdlib.h"
#include <stdint.h>

#define _debug_puts(...) debug_puts_c("VFS/mount/iso9660", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/iso9660", __VA_ARGS__)

#define swipe_endian(w) ((uint16_t)((w >> 8) | (w << 8)))
#define ISO_SECTOR_SIZE 2048

typedef struct {
    dentry_t* device_file;
    dentry_t* target;
    uint16_t* buffer;
    wait_obj_t* wait_obj;
} mount_task_t;

static mount_task_t** tasks = NULL;
static size_t tasks_capacity = 0;
static size_t tasks_count = 0;
static spinlock_t tasks_spinlock = SPINLOCK_INIT;

static size_t alloc_task(dentry_t* device_file, dentry_t* target)
{
    mount_task_t* task = kmalloc_flags(sizeof(mount_task_t), KMALLOC_ZERO);
    task->device_file = device_file;
    task->target = target;
    task->wait_obj = wait_allocate_queue();
    task->buffer = kmalloc(sizeof(uint16_t) * (ISO_SECTOR_SIZE / 2));

    spinlock_acquire(&tasks_spinlock);
    if (tasks_count == tasks_capacity) {
        tasks_capacity++;
        tasks = krealloc(tasks, sizeof(mount_task_t*) * tasks_capacity);
        tasks[tasks_count] = task;
        tasks_count++;
        spinlock_release(&tasks_spinlock);
        return tasks_count - 1;
    }

    for (size_t i = 0; i < tasks_capacity; i++) {
        if (tasks[i] == NULL) {
            tasks[i] = task;
            tasks_count++;
            spinlock_release(&tasks_spinlock);
            return i;
        }
    }

    // This should never happen
    spinlock_release(&tasks_spinlock);
    abort();
}

static void dealloc_task(size_t id)
{
    spinlock_acquire(&tasks_spinlock);
    if (id >= tasks_capacity) {
        spinlock_release(&tasks_spinlock);
        return;
    }
    if (tasks[id] == NULL) {
        spinlock_release(&tasks_spinlock);
        return;
    }

    kfree(tasks[id]->buffer);
    wait_deallocate(tasks[id]->wait_obj);
    kfree(tasks[id]);
    tasks_count--;
}

vfs_mount_result_t vfs_mount_iso9660(
    dentry_t* device_file,
    dentry_t* target __attribute__((unused)),
    uint16_t flags __attribute__((unused)),
    const void* data __attribute__((unused))
) {
    // First check: all ISO9660 files must have a size of at least 0x11 * 2 KiB:
    // - sector size in ISO9660 is 2 KiB
    // - sector 0x10 contains first volume descriptor
    if (device_file->inode->length < (0x11 * ISO_SECTOR_SIZE)) {
        return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
    }

    size_t mount_task_id = alloc_task(device_file, target);

    return MOUNT_ERR_UNKNOWN_FILESYSTEM;
}
