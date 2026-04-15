#include "kernel/memory/kmalloc.h"
#include "kernel/spinlock.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "../mount.h"
#include "stdlib.h"
#include <stdint.h>
#include <string.h>
#include "kernel/debug.h"
#include "./iso9660.h"

#define _debug_puts(...) debug_puts_c("VFS/mount/iso9660", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/iso9660", __VA_ARGS__)

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
    task->buffer = kmalloc(sizeof(uint8_t) * ISO_SECTOR_SIZE);
    task->completed = false;
    task->result = MOUNT_SUCCESS;

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

static bool is_task_completed(void* arg)
{
    mount_task_t* task = arg;
    return task->completed;
}

/**
 * Returns false if data stored currently in buffer is not a valid descriptor
 */
static bool validate_volume_descriptor(mount_task_t* task)
{
    if (task->buffer[1] != 'C' || task->buffer[2] != 'D'
        || task->buffer[3] != '0' || task->buffer[4] != '0'
        || task->buffer[5] != '1'
    ) {
        _debug_puts("Invalid volume descriptor header - missing 'CD001'");
        return false;
    }
    if (task->buffer[6] != 0x01) {
        _debug_puts("Invalid volume descriptor header - wrong version");
        return false;
    }
    return true;
}

static void dump_dirrec(iso_dir_record_t* dirrec)
{
    char* filename = kmalloc(sizeof(char) * dirrec->file_name_length + 1);
    memcpy(filename, dirrec->file_name, dirrec->file_name_length);
    filename[dirrec->file_name_length] = '\0';

    _debug_printf("Dirrec '%s', flags=%x\n", filename, dirrec->file_flags);
    _debug_printf(
        "  rec length=%u, EAR length=%u\n",
        dirrec->directory_record_length,
        dirrec->extended_attribute_record_length
    );
    _debug_printf(
        "  Extent LBA=0x%08X, size=%u\n",
        dirrec->extent_lba,
        dirrec->extent_size
    );

    kfree(filename);
}

static void run_mounting_task(mount_task_t* task)
{
    vfs_file_t* file = vfs_open(task->device_file, VFS_MODE_READONLY);
    vfs_seek(file, 0x10 * ISO_SECTOR_SIZE);
    bool terminator_found = false;
    while (!terminator_found) {
        vfs_read(file, ISO_SECTOR_SIZE, task->buffer);
        if (!validate_volume_descriptor(task)) {
            task->result = MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
            task->completed = true;
            return;
        }
        uint8_t typecode = task->buffer[0];
        _debug_printf("Volume type code: %u\n", typecode);
        if (typecode == 255) {
            _debug_puts("Terminator found");
            terminator_found = true;
        }
        if (typecode == 1) {
            _debug_puts("Primary volume descriptor found");

            iso_dir_record_t dirrec;
            memcpy(&dirrec, task->buffer + 156, 34);
            dump_dirrec(&dirrec);
        }
    }
}

vfs_mount_result_t vfs_mount_iso9660(
    dentry_t* device_file,
    dentry_t* target __attribute__((unused)),
    uint16_t flags __attribute__((unused)),
    const void* data __attribute__((unused))
) {
    char* device_path = kmalloc_flags(VFS_MAX_PATH_LENGTH, KMALLOC_ZERO);
    char* target_path = kmalloc_flags(VFS_MAX_PATH_LENGTH, KMALLOC_ZERO);
    vfs_build_absolute_path(
        vfs_get_real_root(),
        device_file,
        device_path,
        VFS_MAX_PATH_LENGTH
    );
    vfs_build_absolute_path(
        vfs_get_real_root(),
        target,
        target_path,
        VFS_MAX_PATH_LENGTH
    );
    _debug_printf(
        "Attempting to mount device %s at target %s\n",
        device_path,
        target_path
    );
    kfree(device_path);
    kfree(target_path);

    // First check: all ISO9660 files must have a size of at least 0x11 * 2 KiB:
    // - sector size in ISO9660 is 2 KiB
    // - sector 0x10 contains first volume descriptor
    if (device_file->inode->length < (0x11 * ISO_SECTOR_SIZE)) {
        _debug_printf(
            "Device file is too small: %u B\n",
            device_file->inode->length
        );
        return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
    }

    // Create task entry
    size_t mount_task_id = alloc_task(device_file, target);
    mount_task_t* task = tasks[mount_task_id];

    // Run task, then wait for task to be marked as completed
    run_mounting_task(task);
    // TODO: is that waiting queue really needed? In current approach with
    // reading from file via VFS the process is halted anyway.
    wait_on_condition(task->wait_obj, is_task_completed, task);

    // Store result, deallocate memory, and return
    vfs_mount_result_t result = task->result;
    dealloc_task(mount_task_id);
    return result;
}
