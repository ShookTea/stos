#ifndef INCLUDE_KERNEL_VFS_H
#define INCLUDE_KERNEL_VFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Header files for the Virtual File System. VFS is organized as a tree, with
 * a root node. Each node represents either a file, or a directory.
 */

// A normal file
#define VFS_TYPE_FILE 0x01
// A normal directory
#define VFS_TYPE_DIRECTORY 0x02
// TODO: types for devices, mounted directories, etc.

struct vfs_node;
struct dirent;

// Handlers for opening and closing nodes
typedef void (*open_node_t)(struct vfs_node* node, bool read, bool write);
typedef void (*close_node_t)(struct vfs_node* node);
// Handler for reading `size` bytes from node `node`, starting from `offset`,
// and storing them to address at `ptr`. Returns number of read bytes.
typedef size_t (*read_node_t)(
    struct vfs_node* node,
    size_t offset,
    size_t size,
    void* ptr
);
// Handler for writing `size` bytes from address at `ptr` to node `node`,
// starting at offset `offset`. Returns number of written bytes.
typedef size_t (*write_node_t)(
    struct vfs_node* node,
    size_t offset,
    size_t size,
    void* ptr
);
// Handler for reading children of directory. For given directory `node`
// returns name of N'th (`index`) child as a struct dirent, or NULL if no
// child with given index exists.
typedef struct dirent* (*readdir_node_t)(struct vfs_node* node, size_t index);
// Handler for retrurning a child node from current `node` directory, based on
// child's `name`.
typedef struct vfs_node* (*finddir_node_t)(struct vfs_node* node, char* name);

/*
 * Basic definition of a single node in the VFS.
 * TODO: Implementation of ownership and permissions system (owner, group,
 * read/write/execute flags)
 */
typedef struct vfs_node {
    char filename[128];
    uint8_t type; // one of VFS_TYPE_
    uint32_t inode; // File ID, device specific, to identify files (on a disk)
    uint32_t length; // File size in bytes
    open_node_t open_node;
    close_node_t close_node;
    read_node_t read_node;
    write_node_t write_node;
    readdir_node_t readdir_node;
    finddir_node_t finddir_node;
    struct vfs_node* parent_node;
} vfs_node_t;

// One of these is returned by the readdir call, according to POSIX.
struct dirent
{
  char name[128]; // Filename.
  uint32_t ino; // Inode number. Required by POSIX.
};

// The root of the filesystem
extern vfs_node_t* vfs_root;

size_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, void* ptr);
size_t vfs_write(vfs_node_t* node, uint32_t offset, uint32_t size, void* ptr);
void vfs_open(vfs_node_t* node, bool read, bool write);
void vfs_close(vfs_node_t* node);
struct dirent* vfs_readdir(vfs_node_t* node, size_t index);
vfs_node_t* vfs_finddir(vfs_node_t* node, char* name);

#endif
