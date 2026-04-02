#include "kernel/vfs/device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static vfs_node_t* node = NULL;
static vfs_node_t** device_files = NULL;
static size_t device_files_count = 0;

static void add_device_file(vfs_node_t* node)
{
    device_files = krealloc(
        device_files,
        sizeof(vfs_node_t*) * (device_files_count + 1)
    );
    device_files[device_files_count] = node;
    device_files_count++;
}

static struct dirent* readdir(
    vfs_node_t* directory __attribute__((unused)),
    size_t index
) {
    if (index >= device_files_count) {
        return NULL;
    }
    static struct dirent ent;
    strcpy(ent.name, device_files[index]->filename);
    ent.ino = device_files[index]->inode;
    return &ent;
}

static vfs_node_t* finddir(
    vfs_node_t* directory __attribute__((unused)),
    char* name
) {
    for (size_t i = 0; i < device_files_count; i++) {
        if (strcmp(name, device_files[i]->filename) == 0) {
            return device_files[i];
        }
    }
    return NULL;
}

vfs_node_t* device_mount()
{
    if (node != NULL) {
        return node;
    }

    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "dev", VFS_TYPE_DIRECTORY | VFS_TYPE_MOUNTPOINT);
    node->readdir_node = readdir;
    node->finddir_node = finddir;
    vfs_mount_node(node);

    add_device_file(device_tty_mount());

    return node;
}
