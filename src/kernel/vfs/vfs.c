#include <kernel/vfs/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/memory/kmalloc.h>
#include <stdio.h>
#include <string.h>

#define VFS_MAX_MOUNTED_NODES 32

vfs_node_t* vfs_root = 0;
static vfs_node_t** mounted_nodes = NULL;
static uint32_t mounted_notes_count = 0;
static vfs_file_t** open_file_handles = NULL;
static uint32_t open_file_handles_count = 0;

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

size_t vfs_write(vfs_file_t* file, uint32_t size, void* ptr)
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
    // TODO: implementation
    // if ((node->type & VFS_TYPE_DIRECTORY)) {
    //     // TODO: report error?
    //     return NULL;
    // }
    // if (node->open_node != 0) {
    //     return node->open_node(node, read, write);
    // }
    return NULL;
}

vfs_file_t* vfs_close(vfs_node_t *node)
{
    // TODO: implementation
    // if (node->close_node != 0) {
    //     return node->close_node(node);
    // }
    return NULL;
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
    if (mounted_notes_count >= VFS_MAX_MOUNTED_NODES) {
        return;
    }

    mounted_nodes[mounted_notes_count] = node;
    mounted_notes_count++;
}

vfs_node_t* vfs_resolve(char* abs_path)
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
            return NULL;
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
