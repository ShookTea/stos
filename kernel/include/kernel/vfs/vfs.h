#ifndef INCLUDE_KERNEL_VFS_H
#define INCLUDE_KERNEL_VFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Header files for the Virtual File System. VFS is organized as a tree, with
 * a root node. Each node represents either a file, or a directory.
 */

// Max length of a full path.
#define VFS_MAX_PATH_LENGTH 4096

// A normal file
#define VFS_TYPE_FILE 0x01
// A normal directory
#define VFS_TYPE_DIRECTORY 0x02
// Character device file (stream of bytes, handled sequentially)
#define VFS_TYPE_CHARACTER_DEVICE 0x04
// Block device file (allowing reading any block size and alignment)
#define VFS_TYPE_BLOCK_DEVICE 0x08
// Mounted file.
#define VFS_TYPE_MOUNTPOINT 0x10

// Read-only mode
#define VFS_MODE_READONLY 0x01
// Write-only mode
#define VFS_MODE_WRITEONLY 0x02
// Read+write mode
#define VFS_MODE_READWRITE 0x04
// Truncate mode (make the file empty if already exists)
#define VFS_MODE_TRUNCATE 0x08
// Create mode (create a new file if it doesn't exit)
#define VFS_MODE_CREATE 0x10
// Append mode (all writes will be added to the end of file)
#define VFS_MODE_APPEND 0x20
// Mapping of fopen() modes to modes above:
// `r`  = VFS_MODE_READONLY
//     (open file at the beginning, for reading)
// `r+` = VFS_MODE_READWRITE
//     (open file at the beginning, for reading and writing)
// `w`  = VFS_MODE_WRITEONLY | VFS_MODE_CREATE | VFS_MODE_TRUNCATE
//     (truncate file to zero length or create a new one, open for writing)
// `w+` = VFS_MODE_READWRITE | VFS_MODE_CREATE | VFS_MODE_TRUNCATE
//     (truncate file to zero length or create a new one, open for read&write)
// `a`  = VFS_MODE_WRITEONLY | VFS_MODE_CREATE | VFS_MODE_APPEND
//     (create if not exist, open file for appending at the end of file)
// `a+` = VFS_MODE_READWRITE | VFS_MODE_CREATE | VFS_MODE_APPEND
//     (create if not exist, open file for appending at the end of file + read)

struct vfs_node;
struct dirent;

/**
 * Struct for file handle.
 */
typedef struct {
    struct vfs_node* node; // Node pointed by this handle
    uint32_t id; // Handle ID (used internally by VFS)
    uint64_t offset; // Current offset in bytes from the start of file
    bool readable; // Is file readable?
    bool writeable; // Is file writeable?
    void* metadata; // Available to use by the filesystem
} vfs_file_t;

// Handlers for opening and closing nodes
// Can be used for populating metadata by the filesystem
typedef void (*open_node_t)(
    struct vfs_node* node,
    vfs_file_t* file,
    uint8_t mode
);
typedef void (*close_node_t)(struct vfs_node* node, vfs_file_t* file);

// Handler for reading `size` bytes from file `file`, starting from `offset`,
// and storing them to address at `ptr`. Returns number of read bytes.
typedef size_t (*read_node_t)(
    vfs_file_t* file,
    size_t offset,
    size_t size,
    void* ptr
);
// Handler for writing `size` bytes from address at `ptr` to file `file`,
// starting at offset `offset`. Returns number of written bytes.
typedef size_t (*write_node_t)(
    vfs_file_t* file,
    size_t offset,
    size_t size,
    const void* ptr
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
    uint64_t length; // File size in bytes
    uint32_t open_count;
    open_node_t open_node;
    close_node_t close_node;
    read_node_t read_node;
    write_node_t write_node;
    readdir_node_t readdir_node;
    finddir_node_t finddir_node;
    void* metadata; // Data that can be used by filesystem
} vfs_node_t;

// One of these is returned by the readdir call, according to POSIX.
struct dirent
{
  char name[128]; // Filename.
  uint32_t ino; // Inode number. Required by POSIX.
};

// The root of the filesystem
extern vfs_node_t* vfs_root;

size_t vfs_read(vfs_file_t* file, uint32_t size, void* ptr);
size_t vfs_write(vfs_file_t* file, uint32_t size, const void* ptr);
vfs_file_t* vfs_open(vfs_node_t* node, uint8_t open_mode);
void vfs_close(vfs_file_t* file);
struct dirent* vfs_readdir(vfs_node_t* node, size_t index);
vfs_node_t* vfs_finddir(vfs_node_t* node, char* name);

void vfs_init();

/**
 * Adds node to the mounted list. TODO: possibility of unmounting
 */
void vfs_mount_node(vfs_node_t* node);

/**
 * Resolves an absolute path and returns a valid node, or NULL if not found.
 */
vfs_node_t* vfs_resolve(const char* abs_path);

/**
 * Populates the new node with the filename and type, and sets the rest of
 * properties to NULL.
 */
void vfs_populate_node(vfs_node_t* node, char* filename, uint8_t type);

/**
 * Returns the real root node of the VFS (as opposed to the root node as seen
 * by a particular task)
 */
vfs_node_t* vfs_get_real_root_node();

#endif
