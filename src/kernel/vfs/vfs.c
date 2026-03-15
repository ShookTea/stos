#include <kernel/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

vfs_node_t* vfs_root = 0;

size_t read_vfs(vfs_node_t* node, uint32_t offset, uint32_t size, void* ptr)
{
    if (node->read_node != 0) {
        return node->read_node(node, offset, size, ptr);
    }
    return 0;
}

size_t write_vfs(vfs_node_t* node, uint32_t offset, uint32_t size, void* ptr)
{
    if (node->write_node != 0) {
        return node->write_node(node, offset, size, ptr);
    }
    return 0;
}

void open_vfs(vfs_node_t* node, bool read, bool write)
{
    if (node->open_node != 0) {
        node->open_node(node, read, write);
    }
}

void close_vfs(vfs_node_t *node)
{
    if (node->close_node != 0) {
        node->close_node(node);
    }
}

struct dirent* readdir_vfs(vfs_node_t* node, size_t index)
{
    if (node->type == VFS_TYPE_DIRECTORY && node->readdir_node != 0) {
        return node->readdir_node(node, index);
    }
    return NULL;
}

vfs_node_t* finddir_vfs(vfs_node_t* node, char* name)
{
    if (node->type == VFS_TYPE_DIRECTORY && node->finddir_node != 0) {
        return node->finddir_node(node, name);
    }
    return NULL;
}
