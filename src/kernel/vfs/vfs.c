#include <kernel/vfs/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/memory/kmalloc.h>
#include <string.h>

vfs_node_t* vfs_root = 0;

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
    vfs_root->readdir_node = NULL;
    vfs_root->finddir_node = NULL;
}

size_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, void* ptr)
{
    if (node->read_node != 0) {
        return node->read_node(node, offset, size, ptr);
    }
    return 0;
}

size_t vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, void* ptr)
{
    if (node->write_node != 0) {
        return node->write_node(node, offset, size, ptr);
    }
    return 0;
}

void vfs_open(vfs_node_t* node, bool read, bool write)
{
    if (node->open_node != 0) {
        node->open_node(node, read, write);
    }
}

void vfs_close(vfs_node_t *node)
{
    if (node->close_node != 0) {
        node->close_node(node);
    }
}

struct dirent* vfs_readdir(vfs_node_t* node, size_t index)
{
    if (node->type == VFS_TYPE_DIRECTORY && node->readdir_node != 0) {
        return node->readdir_node(node, index);
    }
    return NULL;
}

vfs_node_t* vfs_finddir(vfs_node_t* node, char* name)
{
    if (node->type == VFS_TYPE_DIRECTORY && node->finddir_node != 0) {
        return node->finddir_node(node, name);
    }
    return NULL;
}
