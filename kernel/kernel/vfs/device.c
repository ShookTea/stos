#include "kernel/vfs/device.h"
#include "./device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <stddef.h>
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

static void add_device_files(vfs_node_t** nodes)
{
    while (*nodes != NULL) {
        add_device_file(*nodes);
        nodes++;
    }
}

static bool device_readdir(
    vfs_node_t* directory __attribute__((unused)),
    size_t index,
    struct dirent* out
) {
    if (index < device_files_count) {
        strcpy(out->d_name, device_files[index]->filename);
        out->d_ino = device_files[index]->inode;
        out->d_type = vfs_type_to_dirent(device_files[index]->type);
        return true;
    }
    // Past the static list: enumerate partition nodes dynamically.
    return device_hd_readdir_partition(index - device_files_count, out);
}

static vfs_node_t* device_finddir(
    vfs_node_t* directory __attribute__((unused)),
    char* name
) {
    for (size_t i = 0; i < device_files_count; i++) {
        if (strcmp(name, device_files[i]->filename) == 0) {
            return device_files[i];
        }
    }
    return device_hd_finddir_partition(name);
}

dentry_t* device_mount()
{
    if (node != NULL) {
        return vfs_resolve("/dev");
    }

    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "dev", VFS_TYPE_DIRECTORY | VFS_TYPE_MOUNTPOINT);
    node->readdir_node = device_readdir;
    node->finddir_node = device_finddir;
    dentry_t* dev_dentry = vfs_add_node("dev", node);

    add_device_file(device_tty_mount());
    add_device_files(device_stdio_mount());
    add_device_files(device_hd_mount());

    return dev_dentry;
}

void device_unmount()
{
    if (node == NULL) {
        return;
    }
    device_tty_unmount();
    device_hd_unmount();
    device_stdio_unmount();
    // TODO: call VFS and tell it that the node no longer exists
    kfree(node);
    node = NULL;

    if (device_files != NULL) {
        kfree(device_files);
        device_files = NULL;
        device_files_count = 0;
    }
}
