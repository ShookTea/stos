#ifndef INCLUDE_KERNEL_VFS_H
#define INCLUDE_KERNEL_VFS_H

#include "sys/stat.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>

/**
 * Header files for the Virtual File System. VFS is organized as a tree, with
 * a root node. Each node represents either a file, or a directory.
 */

// Max length of a full path.
#define VFS_MAX_PATH_LENGTH 4096
#define VFS_MAX_FILENAME 256

// A normal file
#define VFS_TYPE_FILE 0x01
// A normal directory
#define VFS_TYPE_DIRECTORY 0x02
// Symbolic link
#define VFS_TYPE_SYMLINK 0x04
// Character device file (stream of bytes, handled sequentially)
#define VFS_TYPE_CHARACTER_DEVICE 0x08
// Block device file (allowing reading any block size and alignment)
#define VFS_TYPE_BLOCK_DEVICE 0x10
// FIFO file (i.e. pipe)
#define VFS_TYPE_FIFO 0x20

#define VFS_PERM_ALL_EXEC    0x0001
#define VFS_PERM_ALL_WRITE   0x0002
#define VFS_PERM_ALL_READ    0x0004
#define VFS_PERM_GROUP_EXEC  0x0008
#define VFS_PERM_GROUP_WRITE 0x0010
#define VFS_PERM_GROUP_READ  0x0020
#define VFS_PERM_OWNER_EXEC  0x0040
#define VFS_PERM_OWNER_WRITE 0x0080
#define VFS_PERM_OWNER_READ  0x0100

struct vfs_node;
struct dentry;

/**
 * A dentry (directory entry) represents a name in the VFS namespace. It links
 * a name and its position in the directory hierarchy to an inode (vfs_node_t).
 * This separation allows parent-pointer traversal, path reconstruction, and a
 * dentry cache that avoids repeated filesystem lookups.
 */
typedef struct dentry {
    char name[VFS_MAX_FILENAME];
    struct dentry* parent;
    struct vfs_node* inode;
} dentry_t;

/**
 * Struct for file handle.
 */
typedef struct {
    dentry_t* dentry; // Dentry pointed by this handle
    uint32_t id; // Handle ID (used internally by VFS)
    uint64_t offset; // Current offset in bytes from the start of file
    bool readable; // Is file readable?
    bool writeable; // Is file writeable?
    void* metadata; // Available to use by the filesystem
    int refcount; // How many references there are to this file handler?
} vfs_file_t;

// Handlers for opening and closing nodes
// Can be used for populating metadata by the filesystem
// Should return `0` on success or value from `errno.h` on failure
typedef int (*open_node_t)(
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
// stores name of N'th (`index`) child as a struct dirent, or returns false if
// no child with given index exists.
typedef bool (*readdir_node_t)(
    struct vfs_node* node,
    size_t index,
    struct dirent* out
);
// Handler for returning a child inode from current `node` directory, based on
// child's `name`. Returns the inode, or NULL if not found. The dentry layer
// wraps the returned inode in a new dentry and caches it.
typedef struct vfs_node* (*finddir_node_t)(struct vfs_node* node, char* name);
// Handler for creating a subdirectory named `name` inside directory `node`.
// Called by vfs_mkdir after the in-memory dentry/inode are created, so the
// filesystem backend can persist the new directory entry on disk.
// Returns 0 on success, one of `errno.h` values on failure.
typedef int (*mkdir_node_t)(struct vfs_node* node, const char* name);
// Handler for creating a file named `name` inside directory `node`. Called by
// vfs_mkdir after the in-memory dentry/inode are created, so the filesystem
// backend can persist the new file entry on disk. Returns 0 on success,
// one of `errno.h` values on failure.
typedef int (*mkfile_node_t)(struct vfs_node* node, const char* name);
// Handler for ioctl syscall commands.
typedef int (*ioctl_node_t)(vfs_file_t* file, uint32_t op, void* arg);
/**
 * Forces saving of all pending changes to the filesystem.
 */
typedef void (*sync_node_t)(const vfs_file_t* file);
/**
 * Populate stat struct with information about node. Return 0 on success; on
 * error return one of possible errors as defined in `errno.h`.
 */
typedef int (*stat_node_t)(const struct vfs_node* node, struct stat* stat);
/**
 * If `node` is a symlink, stores symlink target (either relative or absolute)
 * in `buf`, and returns number of characters stored there. Up to `size`
 * characters will be stored. It won't append NULL at the end.
 *
 * On failure it will return (as negative value) one of possible errors as
 * defined in `errno.h.`.
 */
typedef int (*readlink_node_t)(
    const struct vfs_node* node,
    char* buf,
    size_t size
);
/**
 * Truncates file to zero length. Called by vfs_open when O_TRUNC is set.
 * Returns 0 on success or errno value on failure.
 */
typedef int (*truncate_node_t)(vfs_file_t* file);

/*
 * Basic definition of a single inode in the VFS. Contains file data and
 * filesystem-level operations. Name and hierarchy are managed by dentry_t.
 * TODO: Implementation of ownership and permissions system (owner, group,
 * read/write/execute flags)
 */
typedef struct vfs_node {
    char filename[VFS_MAX_FILENAME]; // Filesystem-internal name (used by filesystem callbacks)
    uint8_t type; // one of VFS_TYPE_
    ino_t inode; // File ID, device specific, to identify files (on a disk)
    uint64_t length; // File size in bytes
    mode_t permissions;
    uint32_t open_count;
    // Called when open_count drops to 0. Set on dynamically-allocated nodes.
    void (*on_release)(struct vfs_node* node);
    open_node_t open_node;
    close_node_t close_node;
    read_node_t read_node;
    write_node_t write_node;
    readdir_node_t readdir_node;
    finddir_node_t finddir_node;
    mkdir_node_t mkdir_node;
    mkfile_node_t mkfile_node;
    ioctl_node_t ioctl_node;
    sync_node_t sync_node;
    stat_node_t stat_node;
    readlink_node_t readlink_node;
    truncate_node_t truncate_node;
    void* metadata; // Data that can be used by filesystem
} vfs_node_t;

// The root dentry of the filesystem
extern dentry_t* vfs_root;

size_t vfs_read(vfs_file_t* file, size_t size, void* ptr);
void vfs_seek(vfs_file_t* file, uint64_t offset);
size_t vfs_write(vfs_file_t* file, size_t size, const void* ptr);
/**
 * On error returns NULL and stores error code in `errno` (if not null)
 */
vfs_file_t* vfs_open(dentry_t* dentry, uint8_t open_mode, int* errno);
void vfs_close(vfs_file_t* file);
bool vfs_readdir(dentry_t* dentry, size_t index, struct dirent* out);
dentry_t* vfs_finddir(dentry_t* dentry, const char* name);
void vfs_sync(const vfs_file_t* file);
int vfs_readlink(const dentry_t* dentry, char* buf, size_t len);
/**
 * Populate `stat` with stats about given `dentry`, and return `0` on success
 * or one of `errno.h` values on failure.
 */
int vfs_stat(const dentry_t* dentry, struct stat* stat);

/**
 * Bumps refcount in given file handler
 */
void vfs_bump_refcount(vfs_file_t* file);

void vfs_init();

/**
 * Mounts an inode under the VFS root with the given name. Returns the created
 * dentry, or NULL if the root is not yet initialized.
 */
dentry_t* vfs_add_node(const char* name, vfs_node_t* inode);

/**
 * Creates a new dentry as a child of `parent`, wrapping `inode`. Used by
 * vfs_mount and the dentry cache to register newly discovered children.
 */
dentry_t* vfs_dentry_create(
    dentry_t* parent,
    const char* name,
    vfs_node_t* inode
);

/**
 * Populates the new node with the filename and type, and sets the rest of
 * properties to NULL/zero.
 */
void vfs_populate_node(vfs_node_t* node, char* filename, uint8_t type);

/**
 * Creates a new directory named `name` as a child of `parent`. The in-memory
 * dentry and inode are always created. If the parent inode has a `mkdir_node`
 * callback set, it is also called so a filesystem backend can persist the entry.
 * Returns 0 on success or one of `errno.h` values on failure.
 */
int vfs_mkdir(dentry_t* parent, const char* name);

/**
 * Creates a new file named `name` as a child of `parent`. The in-memory
 * dentry and inode are always created. If the parent inode has a `mkfile_node`
 * callback set, it is also called so a filesystem backend can persist the entry.
 * Returns 0 on success or one of `errno.h` values on failure.
 */
int vfs_mkfile(dentry_t* parent, const char* name);

/**
 * Returns the real root dentry of the VFS.
 */
dentry_t* vfs_get_real_root();

/**
 * Frees a dentry returned by vfs_finddir or vfs_resolve. If the inode is
 * dynamically owned (on_release set, open_count == 0), fires on_release before
 * freeing the dentry. Safe to call with vfs_root (no-op).
 */
void vfs_dentry_unref(dentry_t* dentry);

typedef enum {
    MOUNT_SUCCESS = 0,
    /** One of required parameters is NULL */
    MOUNT_ERR_NULL_POINTER,
    /** Unrecognized filesystem name */
    MOUNT_ERR_UNKNOWN_FILESYSTEM,
    /** Target is not a directory */
    MOUNT_ERR_TARGET_NOT_DIR,
    /** Device is not in chosen filesystem format */
    MOUNT_ERR_DEVICE_NOT_IN_FORMAT,
} vfs_mount_result_t;

#define MOUNT_FILESYSTEM_PROC "proc"
#define MOUNT_FILESYSTEM_EXT2 "ext2"
#define MOUNT_FILESYSTEM_ISO9660 "iso9660"

/**
 * Attempts to mount given device under a target, using a filesystem with given
 * name.
 * Flags are currently unused and should be set to 0.
 * Data is dependent on the used filesystem.
 */
vfs_mount_result_t vfs_mount(
    dentry_t* device_file,
    dentry_t* target,
    const char* filesystem,
    uint16_t flags,
    const void* data
);

/**
 * Forces synchronization of everything in the filesystem
 */
void vfs_sync_filesystem(void);

inline char vfs_type_to_dirent(uint8_t type)
{
    switch (type) {
        case VFS_TYPE_FILE: return DT_REG;
        case VFS_TYPE_DIRECTORY: return DT_DIR;
        case VFS_TYPE_SYMLINK: return DT_LNK;
        case VFS_TYPE_CHARACTER_DEVICE: return DT_CHR;
        case VFS_TYPE_BLOCK_DEVICE: return DT_BLK;
        case VFS_TYPE_FIFO: return DT_FIFO;
        default: return DT_UNKNOWN;
    }
}

#endif
