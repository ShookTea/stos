#include <kernel/vfs/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/memory/kmalloc.h>
#include <string.h>

#define VFS_MAX_MOUNTED_NODES 32

vfs_node_t* vfs_root = 0;
vfs_node_t** mounted_nodes = NULL;
uint32_t mounted_notes_count = 0;

static struct dirent* vfs_root_readdir(
    __attribute__((unused))struct vfs_node* node,
    size_t index
) {
    if (index > mounted_notes_count) {
        return NULL;
    }
    static struct dirent ent;
    strcpy(ent.name, mounted_nodes[index]->filename);
    return &ent;
}

static vfs_node_t* vfs_root_finddir(
    __attribute__((unused))struct vfs_node* node,
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

void vfs_mount_node(vfs_node_t* node)
{
    if (mounted_notes_count >= VFS_MAX_MOUNTED_NODES) {
        return;
    }

    mounted_nodes[mounted_notes_count] = node;
    mounted_notes_count++;
}
