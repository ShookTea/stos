#include "kernel/spinlock.h"
#include <kernel/vfs/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/memory/kmalloc.h>
#include <string.h>

#define VFS_MAX_MOUNTED_NODES 32
// By how many entries should we extend file_handles array if needed?
#define VFS_FILE_HANDLE_REALLOC_SIZE 10

vfs_node_t* vfs_root = 0;
static vfs_node_t** mounted_nodes = NULL;
static uint32_t mounted_notes_count = 0;

// Allocation of some memory for existing file handles
static vfs_file_t** file_handles = NULL;
// Size of file_handles
static uint32_t file_handles_size = 0;
// How many entries in file_handles are currently present?
// - if smaller than file_handles_size - can reuse some handle
// - if equal to file_handles_size, we need to increase size of file_handles
static uint32_t file_handles_open_count = 0;

static spinlock_t vfs_lock = SPINLOCK_INIT;

/**
 * Allocate a new entry for file handle.
 */
static vfs_file_t* allocate_file_handle(
    vfs_node_t* node,
    uint8_t mode
) {
    spinlock_acquire(&vfs_lock);
    vfs_file_t* handle = kmalloc_flags(sizeof(vfs_file_t), KMALLOC_ZERO);
    handle->node = node;
    handle->readable = mode & (VFS_MODE_READONLY | VFS_MODE_READWRITE);
    handle->writeable = mode & (VFS_MODE_WRITEONLY | VFS_MODE_READWRITE);
    handle->offset = (mode & VFS_MODE_APPEND) ? node->length : 0;
    // TODO: handle VFS_MODE_TRUNCATE and VFS_MODE_CREATE

    if (file_handles_open_count == file_handles_size) {
        // The file_handles is full - we should increase the size of it
        uint32_t new_size = file_handles_size + VFS_FILE_HANDLE_REALLOC_SIZE;
        file_handles = krealloc(
            file_handles,
            sizeof(vfs_file_t*) * new_size
        );
        // Clear newly allocated memory to zero it out (which is used later
        // for checking if handle is present or not)
        memset(file_handles + file_handles_size, 0, VFS_FILE_HANDLE_REALLOC_SIZE);
        file_handles_size = new_size;

        // file_handles_open_count is now guaranteed to point to the first
        // empty item in the array - use it for the new handle
        file_handles[file_handles_open_count] = handle;
        handle->id = file_handles_open_count;
        file_handles_open_count++;
        spinlock_release(&vfs_lock);
        return handle;
    } else {
        // There is some entry in the file_handles array that is empty and
        // ready to be allocated.
        for (size_t i = 0; i < file_handles_size; i++) {
            if (file_handles[i] == NULL) {
                file_handles[i] = handle;
                handle->id = i;
                file_handles_open_count++;
                break;
            }
        }
    }

    spinlock_release(&vfs_lock);
    return handle;
}

/**
 * Deallocates the file handle and removes it from the array
 */
static void deallocate_file_handle(vfs_file_t* handle)
{
    spinlock_acquire(&vfs_lock);
    file_handles[handle->id] = NULL;
    file_handles_open_count--;
    spinlock_release(&vfs_lock);
    kfree(handle);
}

static struct dirent* vfs_root_readdir(
    __attribute__((unused))vfs_node_t* node,
    size_t index
) {
    if (index >= mounted_notes_count) {
        return NULL;
    }
    static struct dirent ent;
    strcpy(ent.name, mounted_nodes[index]->filename);
    ent.ino = mounted_nodes[index]->inode;
    return &ent;
}

static vfs_node_t* vfs_root_finddir(
    __attribute__((unused))vfs_node_t* node,
    char* name
) {
    for (size_t i = 0; i < mounted_notes_count; i++) {
        if (strcmp(mounted_nodes[i]->filename, name) == 0) {
            return mounted_nodes[i];
        }
    }
    return NULL;
}

void vfs_init()
{
    vfs_root = kmalloc(sizeof(vfs_node_t));
    strcpy(vfs_root->filename, "");
    vfs_root->type = VFS_TYPE_DIRECTORY;
    vfs_root->length = 0;
    vfs_root->metadata = NULL;
    vfs_root->open_node = NULL;
    vfs_root->close_node = NULL;
    vfs_root->read_node = NULL;
    vfs_root->write_node = NULL;
    vfs_root->readdir_node = vfs_root_readdir;
    vfs_root->finddir_node = vfs_root_finddir;

    mounted_nodes = kmalloc(sizeof(vfs_node_t*) * VFS_MAX_MOUNTED_NODES);
}

size_t vfs_read(vfs_file_t* file, uint32_t size, void* ptr)
{
    if (file->node->read_node != NULL) {
        size_t bytes = file->node->read_node(file, file->offset, size, ptr);
        if (bytes > 0) {
            file->offset += bytes;
        }
        return bytes;
    }
    return 0;
}

size_t vfs_write(vfs_file_t* file, uint32_t size, const void* ptr)
{
    if (file->node->write_node != 0) {
        size_t bytes = file->node->write_node(file, file->offset, size, ptr);
        if (bytes > 0) {
            file->offset += bytes;
        }
        return bytes;
    }
    return 0;
}

vfs_file_t* vfs_open(vfs_node_t* node, uint8_t mode)
{
    if ((node->type & VFS_TYPE_DIRECTORY)) {
        // TODO: report error?
        return NULL;
    }
    node->open_count++;
    // TODO: this is the place where we could introduce some kind of a lock
    // mechanism for multiple write prevention (if necessary)

    vfs_file_t* handle = allocate_file_handle(node, mode);
    // Allow specific file system to populate metadata
    if (node->open_node != NULL) {
        node->open_node(node, handle, mode);
    }

    return handle;
}

void vfs_close(vfs_file_t* file)
{
    // Allow specific file system to clear metadata
    if (file->node->close_node != NULL) {
        file->node->close_node(file->node, file);
    }
    file->node->open_count--;
    deallocate_file_handle(file);
}

struct dirent* vfs_readdir(vfs_node_t* node, size_t index)
{
    if ((node->type & VFS_TYPE_DIRECTORY) != 0 && node->readdir_node != 0) {
        return node->readdir_node(node, index);
    }
    return NULL;
}

vfs_node_t* vfs_finddir(vfs_node_t* node, char* name)
{
    if ((node->type & VFS_TYPE_DIRECTORY) != 0 && node->finddir_node != 0) {
        return node->finddir_node(node, name);
    }
    return NULL;
}

void vfs_mount_node(vfs_node_t* node)
{
    spinlock_acquire(&vfs_lock);

    if (mounted_notes_count >= VFS_MAX_MOUNTED_NODES) {
        return;
    }

    mounted_nodes[mounted_notes_count] = node;
    mounted_notes_count++;

    spinlock_release(&vfs_lock);
}

vfs_node_t* vfs_resolve(const char* abs_path)
{
    // Make copy of the path (include space for null terminator)
    size_t path_len = strlen(abs_path);
    char* path_copy = kmalloc(sizeof(char) * (path_len + 1));
    strcpy(path_copy, abs_path);

    // Count the number of path parts (separated by '/')
    size_t part_count = 0;
    for (size_t i = 0; i < path_len; i++) {
        if (path_copy[i] == '/' && i + 1 < path_len && path_copy[i + 1] != '/') {
            part_count++;
        }
    }

    // Allocate array to hold pointers to each part
    char** parts = kmalloc(sizeof(char*) * part_count);

    // Split the path by replacing '/' with '\0' and storing pointers
    size_t part_index = 0;
    bool in_part = false;

    for (size_t i = 0; i < path_len; i++) {
        if (path_copy[i] == '/') {
            path_copy[i] = '\0';
            in_part = false;
        } else if (!in_part) {
            // Start of a new part
            parts[part_index++] = &path_copy[i];
            in_part = true;
        }
    }

    vfs_node_t* current_node = vfs_root;

    for (size_t i = 0; i < part_count; i++) {
        current_node = vfs_finddir(current_node, parts[i]);
        if (current_node == NULL) {
            break;
        }
    }

    // Clean up
    kfree(parts);
    kfree(path_copy);

    return current_node;
}

void vfs_populate_node(vfs_node_t* node, char* filename, uint8_t type)
{
    strcpy(node->filename, filename);
    node->type = type;
    node->length = 0;
    node->open_node = NULL;
    node->close_node = NULL;
    node->read_node = NULL;
    node->write_node = NULL;
    node->readdir_node = NULL;
    node->finddir_node = NULL;
    node->metadata = NULL;
}

vfs_node_t* vfs_get_real_root_node()
{
    return vfs_root;
}
