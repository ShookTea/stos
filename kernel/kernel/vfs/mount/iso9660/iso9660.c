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
    tasks[id] = NULL;
    tasks_count--;
    spinlock_release(&tasks_spinlock);
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

static bool iso_readdir(
    struct vfs_node* node,
    size_t index,
    struct dirent* out
) {
    iso9660_node_t* meta = node->metadata;
    _debug_printf(
        "iso_readdir called for node='%s' index='%u'\n",
        node->filename,
        index
    );
    index += 2; // Skip "." and ".." entries

    vfs_file_t* dev = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (dev == NULL) {
        _debug_puts("Device file doesn't exist anymore.");
        return false;
    }
    vfs_seek(dev, meta->extent_lba * ISO_SECTOR_SIZE);
    uint8_t* buffer = kmalloc_flags(ISO_SECTOR_SIZE, KMALLOC_ZERO);
    vfs_read(dev, ISO_SECTOR_SIZE, buffer);
    iso_dir_record_t* dirrec;

    size_t buffer_start_index = 0;
    bool found = false;

    // TODO: in case entry spans multiple sectors, we need to load other sectors
    for (size_t i = 0; i <= index; i++) {
        uint8_t entrysize = buffer[buffer_start_index];
        if (entrysize == 0) {
            break;
        }
        if ((buffer_start_index + entrysize) > ISO_SECTOR_SIZE) {
            // TODO: that probably means that there's a continuation in the
            // next sector.
            break;
        }
        dirrec = (iso_dir_record_t*)(buffer + buffer_start_index);
        buffer_start_index += entrysize;

        if (i == index) {
            _debug_printf("Index %u found\n", i);
            dump_dirrec(dirrec);
            out->ino = i;
            // Copy filename to out
            char* filename = kmalloc_flags(
                sizeof(char) * (dirrec->file_name_length + 1),
                KMALLOC_ZERO
            );
            memcpy(filename, dirrec->file_name, dirrec->file_name_length);
            // Search for first semicolon and replace it with null character
            for (size_t i = 0; i < dirrec->file_name_length; i++) {
                if (filename[i] == ';') {
                    filename[i] = '\0';
                    break;
                }
            }
            strcpy(out->name, filename);
            kfree(filename);
            found = true;
        }
    }

    if (!found) {
        _debug_puts("Index not found.");
    }

    kfree(buffer);
    vfs_close(dev);
    return found;
}

static vfs_node_t* iso_finddir(vfs_node_t* node, char* name)
{
    iso9660_node_t* meta = node->metadata;
    _debug_printf(
        "iso_finddir called for node='%s' name='%s'\n",
        node->filename,
        name
    );

    vfs_file_t* dev = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (dev == NULL) {
        _debug_puts("Device file doesn't exist anymore.");
        return NULL;
    }
    vfs_seek(dev, meta->extent_lba * ISO_SECTOR_SIZE);
    uint8_t* buffer = kmalloc_flags(ISO_SECTOR_SIZE, KMALLOC_ZERO);
    vfs_read(dev, ISO_SECTOR_SIZE, buffer);
    iso_dir_record_t* dirrec;

    size_t buffer_start_index = 0;
    vfs_node_t* found_node = NULL;

    // TODO: in case entry spans multiple sectors, we need to load other sectors
    while (found_node == NULL) {
        uint8_t entrysize = buffer[buffer_start_index];
        if (entrysize == 0) {
            break;
        }
        if ((buffer_start_index + entrysize) > ISO_SECTOR_SIZE) {
            // TODO: that probably means that there's a continuation in the
            // next sector.
            break;
        }
        dirrec = (iso_dir_record_t*)(buffer + buffer_start_index);
        buffer_start_index += entrysize;

        // Copy filename
        char* filename = kmalloc_flags(
            sizeof(char) * (dirrec->file_name_length + 1),
            KMALLOC_ZERO
        );
        memcpy(filename, dirrec->file_name, dirrec->file_name_length);
        // Search for first semicolon and replace it with null character
        for (size_t i = 0; i < dirrec->file_name_length; i++) {
            if (filename[i] == ';') {
                filename[i] = '\0';
                break;
            }
        }

        if (!strcmp(filename, name)) {
            // Found the node!
            _debug_puts("Found the node");
            found_node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
            bool is_subdir = dirrec->file_flags & ISO_DIR_FLAG_SUBDIRECTORY;
            vfs_populate_node(
                found_node,
                filename,
                is_subdir ? VFS_TYPE_DIRECTORY : VFS_TYPE_FILE
            );
            iso9660_node_t* node_meta = kmalloc_flags(
                sizeof(iso9660_node_t),
                KMALLOC_ZERO
            );
            node_meta->device_file = meta->device_file;
            node_meta->extent_lba = dirrec->extent_lba;
            node_meta->extent_size = dirrec->extent_size;
            found_node->metadata = node_meta;

            if (is_subdir) {
                found_node->readdir_node = iso_readdir;
                found_node->finddir_node = iso_finddir;
            }
        }

        kfree(filename);
    }

    if (found_node == NULL) {
        _debug_puts("File not found.");
    }

    kfree(buffer);
    vfs_close(dev);
    return found_node;
}

static void run_mounting_task(mount_task_t* task)
{
    vfs_file_t* file = vfs_open(task->device_file, VFS_MODE_READONLY);
    if (file == NULL) {
        _debug_puts("File doesn't exit!");
        task->result = MOUNT_ERR_NULL_POINTER;
        task->completed = true;
        return;
    }
    vfs_seek(file, 0x10 * ISO_SECTOR_SIZE);
    bool terminator_found = false;
    while (!terminator_found) {
        vfs_read(file, ISO_SECTOR_SIZE, task->buffer);
        if (!validate_volume_descriptor(task)) {
            task->result = MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
            task->completed = true;
            vfs_close(file);
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
            vfs_node_t* inode = task->target->inode;
            inode->type |= VFS_TYPE_MOUNTPOINT;
            iso9660_node_t* meta = kmalloc(sizeof(iso9660_node_t));
            meta->device_file = task->device_file;
            meta->extent_lba = dirrec.extent_lba;
            meta->extent_size = dirrec.extent_size;
            inode->metadata = meta;
            inode->readdir_node = iso_readdir;
            inode->finddir_node = iso_finddir;
            dump_dirrec(&dirrec);
        }
    }

    task->completed = true;
    vfs_close(file);
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
