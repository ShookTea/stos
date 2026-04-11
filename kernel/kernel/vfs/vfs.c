#include "kernel/spinlock.h"
#include <kernel/vfs/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/memory/kmalloc.h>
#include <string.h>

// By how many entries should we extend file_handles array if needed?
#define VFS_FILE_HANDLE_REALLOC_SIZE 10

dentry_t* vfs_root = NULL;

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
    dentry_t* dentry,
    uint8_t mode
) {
    spinlock_acquire(&vfs_lock);
    vfs_file_t* handle = kmalloc_flags(sizeof(vfs_file_t), KMALLOC_ZERO);
    handle->dentry = dentry;
    handle->readable = mode & (VFS_MODE_READONLY | VFS_MODE_READWRITE);
    handle->writeable = mode & (VFS_MODE_WRITEONLY | VFS_MODE_READWRITE);
    handle->offset = (mode & VFS_MODE_APPEND) ? dentry->inode->length : 0;
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
        memset(
            file_handles + file_handles_size,
            0,
            sizeof(vfs_file_t*) * VFS_FILE_HANDLE_REALLOC_SIZE
        );
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

void vfs_init()
{
    vfs_node_t* root_inode = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
    vfs_populate_node(root_inode, "", VFS_TYPE_DIRECTORY);
    // Root directory has no readdir_node/finddir_node: all children are
    // registered via vfs_mount and stored directly in the root dentry's
    // children array, so the dentry cache fallback in vfs_readdir handles it.

    vfs_root = kmalloc_flags(sizeof(dentry_t), KMALLOC_ZERO);
    vfs_root->name[0] = '\0';
    vfs_root->parent = vfs_root; // root's parent is itself (for ".." handling)
    vfs_root->inode = root_inode;
    vfs_root->children = NULL;
    vfs_root->children_count = 0;
}

dentry_t* vfs_dentry_create(
    dentry_t* parent,
    const char* name,
    vfs_node_t* inode
) {
    dentry_t* d = kmalloc_flags(sizeof(dentry_t), KMALLOC_ZERO);
    strncpy(d->name, name, sizeof(d->name) - 1);
    d->parent = parent;
    d->inode = inode;
    d->children = NULL;
    d->children_count = 0;

    if (parent != NULL) {
        spinlock_acquire(&vfs_lock);
        parent->children = krealloc(
            parent->children,
            sizeof(dentry_t*) * (parent->children_count + 1)
        );
        parent->children[parent->children_count] = d;
        parent->children_count++;
        spinlock_release(&vfs_lock);
    }

    return d;
}

dentry_t* vfs_mount(const char* name, vfs_node_t* inode)
{
    if (vfs_root == NULL) {
        return NULL;
    }
    return vfs_dentry_create(vfs_root, name, inode);
}

size_t vfs_read(vfs_file_t* file, size_t size, void* ptr)
{
    vfs_node_t* node = file->dentry->inode;
    if (node->read_node != NULL) {
        size_t bytes = node->read_node(file, file->offset, size, ptr);
        if (bytes > 0) {
            file->offset += bytes;
        }
        return bytes;
    }
    return 0;
}

size_t vfs_write(vfs_file_t* file, size_t size, const void* ptr)
{
    vfs_node_t* node = file->dentry->inode;
    if (node->write_node != NULL) {
        size_t bytes = node->write_node(file, file->offset, size, ptr);
        if (bytes > 0) {
            file->offset += bytes;
        }
        return bytes;
    }
    return 0;
}

vfs_file_t* vfs_open(dentry_t* dentry, uint8_t mode)
{
    vfs_node_t* node = dentry->inode;
    if ((node->type & VFS_TYPE_DIRECTORY)) {
        // TODO: report error?
        return NULL;
    }
    node->open_count++;
    // TODO: this is the place where we could introduce some kind of a lock
    // mechanism for multiple write prevention (if necessary)

    vfs_file_t* handle = allocate_file_handle(dentry, mode);
    // Allow specific file system to populate metadata
    if (node->open_node != NULL) {
        node->open_node(node, handle, mode);
    }

    return handle;
}

void vfs_close(vfs_file_t* file)
{
    vfs_node_t* node = file->dentry->inode;
    // Allow specific file system to clear metadata
    if (node->close_node != NULL) {
        node->close_node(node, file);
    }
    node->open_count--;
    deallocate_file_handle(file);
}

bool vfs_readdir(dentry_t* dentry, size_t index, struct dirent* out)
{
    vfs_node_t* inode = dentry->inode;
    if ((inode->type & VFS_TYPE_DIRECTORY) == 0) {
        return false;
    }

    // If the inode has a readdir handler, use it (filesystem-backed directory)
    if (inode->readdir_node != NULL) {
        return inode->readdir_node(inode, index, out);
    }

    // Fallback: iterate pre-populated dentry children (e.g. VFS root, whose
    // children are registered via vfs_mount rather than a filesystem handler)
    if (index >= dentry->children_count) {
        return false;
    }
    strncpy(out->name, dentry->children[index]->name, sizeof(out->name) - 1);
    out->name[sizeof(out->name) - 1] = '\0';
    out->ino = dentry->children[index]->inode
        ? dentry->children[index]->inode->inode
        : 0;
    return true;
}

dentry_t* vfs_finddir(dentry_t* dentry, const char* name)
{
    vfs_node_t* inode = dentry->inode;
    if ((inode->type & VFS_TYPE_DIRECTORY) == 0) {
        return NULL;
    }

    // 1. Check dentry cache
    spinlock_acquire(&vfs_lock);
    for (size_t i = 0; i < dentry->children_count; i++) {
        if (strcmp(dentry->children[i]->name, name) == 0) {
            spinlock_release(&vfs_lock);
            return dentry->children[i];
        }
    }
    spinlock_release(&vfs_lock);

    // 2. Cache miss: ask the filesystem for the inode (no lock held; the
    //    filesystem callback may block or be slow)
    if (inode->finddir_node == NULL) {
        return NULL;
    }
    vfs_node_t* child_inode = inode->finddir_node(inode, (char*)name);
    if (child_inode == NULL) {
        return NULL;
    }

    // 3. Re-check cache under lock (another thread may have populated it while
    //    we were in the filesystem callback), then insert if still absent
    spinlock_acquire(&vfs_lock);
    for (size_t i = 0; i < dentry->children_count; i++) {
        if (strcmp(dentry->children[i]->name, name) == 0) {
            spinlock_release(&vfs_lock);
            return dentry->children[i];
        }
    }

    dentry_t* child = kmalloc_flags(sizeof(dentry_t), KMALLOC_ZERO);
    strncpy(child->name, name, sizeof(child->name) - 1);
    child->parent = dentry;
    child->inode = child_inode;
    child->children = NULL;
    child->children_count = 0;

    dentry->children = krealloc(
        dentry->children,
        sizeof(dentry_t*) * (dentry->children_count + 1)
    );
    dentry->children[dentry->children_count] = child;
    dentry->children_count++;

    spinlock_release(&vfs_lock);
    return child;
}

dentry_t* vfs_resolve(const char* abs_path)
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

    dentry_t* current = vfs_root;

    for (size_t i = 0; i < part_count; i++) {
        current = vfs_finddir(current, parts[i]);
        if (current == NULL) {
            break;
        }
    }

    // Clean up
    kfree(parts);
    kfree(path_copy);

    return current;
}

dentry_t* vfs_resolve_relative(
    dentry_t* root,
    dentry_t* current,
    const char* path
) {
    if (path == NULL || path[0] == '\0') {
        return current;
    }

    // Absolute path: resolve from root, not vfs_root
    dentry_t* node = (path[0] == '/') ? root : current;

    size_t path_len = strlen(path);
    char* path_copy = kmalloc(path_len + 1);
    strcpy(path_copy, path);

    char* ptr = path_copy;
    while (*ptr != '\0' && node != NULL) {
        // Skip slashes
        while (*ptr == '/') ptr++;
        if (*ptr == '\0') break;

        // Isolate the next component
        char* end = ptr;
        while (*end != '\0' && *end != '/') end++;
        char saved = *end;
        *end = '\0';

        if (strcmp(ptr, ".") == 0) {
            // Stay in current directory
        } else if (strcmp(ptr, "..") == 0) {
            // Go to parent, but never above root
            if (node != root) {
                node = node->parent;
                if (node == NULL) {
                    node = root; // Safety: clamp if tree is malformed
                }
            }
        } else {
            node = vfs_finddir(node, ptr);
        }

        *end = saved;
        ptr = end;
    }

    kfree(path_copy);
    return node;
}

void vfs_populate_node(vfs_node_t* node, char* filename, uint8_t type)
{
    strcpy(node->filename, filename);
    node->type = type;
    node->length = 0;
    node->open_count = 0;
    node->inode = 0;
    node->open_node = NULL;
    node->close_node = NULL;
    node->read_node = NULL;
    node->write_node = NULL;
    node->readdir_node = NULL;
    node->finddir_node = NULL;
    node->metadata = NULL;
}

dentry_t* vfs_get_real_root()
{
    return vfs_root;
}
