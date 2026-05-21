#include "../device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <stdio.h>

typedef struct {
    size_t fd;
} stdio_meta_t;

static vfs_node_t** mounted = NULL;

static void stdio_on_release(vfs_node_t* node)
{
    kfree(node->metadata);
    node->metadata = NULL;
}

static int stdio_readlink(const vfs_node_t* node, char* buf, size_t size)
{
    stdio_meta_t* meta = node->metadata;
    return snprintf(buf, size, "/proc/self/%u", meta->fd);
}

static vfs_node_t* stdio_create_node(size_t fd, char* name)
{
    stdio_meta_t* meta = kmalloc(sizeof(stdio_meta_t));
    meta->fd = fd;

    vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
    vfs_populate_node(node, name, VFS_TYPE_SYMLINK);
    node->metadata = meta;
    node->on_release = stdio_on_release;
    node->readlink_node = stdio_readlink;

    return node;
}

vfs_node_t** device_stdio_mount()
{
    if (mounted != NULL) {
        return mounted;
    }

    mounted = kmalloc(sizeof(vfs_node_t*) * 4);
    mounted[0] = stdio_create_node(0, "stdin");
    mounted[1] = stdio_create_node(1, "stdout");
    mounted[2] = stdio_create_node(2, "stderr");
    mounted[3] = NULL;

    return mounted;
}

void device_stdio_unmount()
{
    if (mounted == NULL) {
        return;
    }

    vfs_node_t** ptr = mounted;
    while (*ptr != NULL) {
        vfs_node_t* node = *ptr;
        stdio_meta_t* meta = node->metadata;
        if (meta != NULL) {
            kfree(meta);
        }
        kfree(node);
        ptr++;
    }
    kfree(mounted);
    mounted = NULL;
}
