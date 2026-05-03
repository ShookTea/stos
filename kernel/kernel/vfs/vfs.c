#include "kernel/spinlock.h"
#include <kernel/vfs/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/memory/kmalloc.h>
#include <string.h>
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c("VFS", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS", __VA_ARGS__)

typedef struct {
    char* filename;
    dentry_t* dentry;
} vfs_root_content_t;

static vfs_root_content_t* root_content = NULL;
static size_t root_content_size = 0;

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

static bool root_readdir(
    struct vfs_node* node __attribute__((unused)),
    size_t index,
    struct dirent* out
) {
    if (index >= root_content_size) {
        return false;
    }
    strcpy(out->name, root_content[index].filename);
    out->ino = root_content[index].dentry->inode->inode;
    return true;
}

static vfs_node_t* root_finddir(
    vfs_node_t* node __attribute__((unused)),
    char* name
) {
    for (size_t i = 0; i < root_content_size; i++) {
        if (strcmp(name, root_content[i].filename) == 0) {
            return root_content[i].dentry->inode;
        }
    }
    return NULL;
}

void vfs_init()
{
    vfs_node_t* root_inode = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
    vfs_populate_node(root_inode, "", VFS_TYPE_DIRECTORY);
    root_inode->readdir_node = root_readdir;
    root_inode->finddir_node = root_finddir;

    vfs_root = kmalloc_flags(sizeof(dentry_t), KMALLOC_ZERO);
    vfs_root->name[0] = '\0';
    vfs_root->parent = vfs_root; // root's parent is itself (for ".." handling)
    vfs_root->inode = root_inode;
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
    return d;
}

static void add_entry_to_root_content(const char* name, dentry_t* dentry)
{
    root_content = krealloc(
        root_content,
        sizeof(vfs_root_content_t) * (root_content_size + 1)
    );

    root_content[root_content_size].filename = kmalloc(
        sizeof(char) * (strlen(name) + 1)
    );
    strcpy(
        root_content[root_content_size].filename,
        name
    );

    root_content[root_content_size].dentry = dentry;
    dentry->inode->open_count++; // root_content holds a permanent reference

    root_content_size++;
}

dentry_t* vfs_add_node(const char* name, vfs_node_t* inode)
{
    if (vfs_root == NULL) {
        return NULL;
    }
    dentry_t* dentry = vfs_dentry_create(vfs_root, name, inode);
    add_entry_to_root_content(name, dentry);
    return dentry;
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

void vfs_seek(vfs_file_t* file, uint64_t offset)
{
    file->offset = offset;
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
    if (node->open_count == 0 && node->on_release != NULL) {
        node->on_release(node);
    }
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

    return false;
}

dentry_t* vfs_finddir(dentry_t* dentry, const char* name)
{
    vfs_node_t* inode = dentry->inode;
    if ((inode->type & VFS_TYPE_DIRECTORY) == 0) {
        return NULL;
    }

    // Ask the filesystem for the inode (no lock held; the
    //    filesystem callback may block or be slow)
    if (inode->finddir_node == NULL) {
        return NULL;
    }
    vfs_node_t* child_inode = inode->finddir_node(inode, (char*)name);
    if (child_inode == NULL) {
        return NULL;
    }

    dentry_t* child = kmalloc_flags(sizeof(dentry_t), KMALLOC_ZERO);
    strncpy(child->name, name, sizeof(child->name) - 1);
    child->parent = dentry;
    child->inode = child_inode;

    return child;
}

void vfs_sync(const vfs_file_t* file)
{
    if (file == NULL || file->dentry == NULL || file->dentry->inode == NULL)
    {
        return;
    }
    vfs_node_t* node = file->dentry->inode;
    if (node->sync_node != NULL) {
        node->sync_node(file);
    }
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
        dentry_t* parent = current;
        current = vfs_finddir(parent, parts[i]);
        if (parent != vfs_root) {
            kfree(parent);
        }
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
    _debug_printf(
        "Resolving relative %s for root=%s current=%s\n",
        path,
        root->name,
        current->name
    );
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

char* vfs_build_absolute_path(
    dentry_t* root,
    dentry_t* current,
    char* buff,
    size_t size
) {
    char** parts = NULL;
    size_t parts_count = 0;
    size_t total_length = 0; // excluding file separators
    while (current != root) {
        parts_count++;
        total_length += sizeof(current->name);
        parts = krealloc(parts, sizeof(char*) * parts_count);
        parts[parts_count - 1] = current->name;
        current = current->parent;
    }
    size_t sep_count = parts_count == 0 ? 1 : parts_count;
    if ((sep_count + total_length + 1) > size) {
        if (parts != NULL) kfree(parts);
        return NULL;
    }

    if (parts_count == 0) {
        buff[0] = '/';
        buff[1] = 0;
        // No need for kfree if parts_count == 0
        return buff;
    }

    size_t buff_index = 0;
    for (int i = parts_count - 1; i >= 0; i--) {
        buff[buff_index] = '/';
        buff_index++;
        strcpy(buff + buff_index, parts[i]);
        buff_index += strlen(parts[i]);
    }
    kfree(parts);
    return buff;
}

void vfs_populate_node(vfs_node_t* node, char* filename, uint8_t type)
{
    strcpy(node->filename, filename);
    node->type = type;
    node->length = 0;
    node->open_count = 0;
    node->on_release = NULL;
    node->inode = 0;
    node->open_node = NULL;
    node->close_node = NULL;
    node->read_node = NULL;
    node->write_node = NULL;
    node->readdir_node = NULL;
    node->finddir_node = NULL;
    node->mkdir_node = NULL;
    node->ioctl_node = NULL;
    node->sync_node = NULL;
    node->metadata = NULL;
}

dentry_t* vfs_mkdir(dentry_t* parent, const char* name)
{
    if (parent == NULL) {
        return NULL;
    }
    if ((parent->inode->type & VFS_TYPE_DIRECTORY) == 0) {
        return NULL;
    }

    vfs_node_t* inode = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
    vfs_populate_node(inode, (char*)name, VFS_TYPE_DIRECTORY);

    dentry_t* dentry = vfs_dentry_create(parent, name, inode);

    // If the parent filesystem provides a mkdir hook, call it so the directory
    // can be persisted on disk once a real filesystem is implemented.
    if (parent->inode->mkdir_node != NULL) {
        parent->inode->mkdir_node(parent->inode, name);
    }

    if (parent == vfs_root) {
        add_entry_to_root_content(name, dentry);
    }

    return dentry;
}

dentry_t* vfs_get_real_root()
{
    return vfs_root;
}

void vfs_dentry_unref(dentry_t* dentry)
{
    if (dentry == NULL || dentry == vfs_root) {
        return;
    }
    vfs_node_t* inode = dentry->inode;
    kfree(dentry);
    if (inode != NULL && inode->on_release != NULL && inode->open_count == 0) {
        inode->on_release(inode);
    }
}
