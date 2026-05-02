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
#define VFS_MAX_FILENAME 256

// A normal file
#define VFS_TYPE_FILE 0x01
// A normal directory
#define VFS_TYPE_DIRECTORY 0x02
// Symbolic link (not supported yet)
#define VFS_TYPE_SYMLINK 0x04
// Character device file (stream of bytes, handled sequentially)
#define VFS_TYPE_CHARACTER_DEVICE 0x08
// Block device file (allowing reading any block size and alignment)
#define VFS_TYPE_BLOCK_DEVICE 0x10
// Mounted file.
#define VFS_TYPE_MOUNTPOINT 0x20

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
// Returns true on success, false on failure.
typedef bool (*mkdir_node_t)(struct vfs_node* node, const char* name);
// Handler for ioctl syscall commands.
typedef int (*ioctl_node_t)(vfs_file_t* file, uint32_t op, void* arg);
/**
 * Forces saving of all pending changes to the filesystem.
 */
typedef void (*sync_node_t)(const vfs_file_t* file);

/*
 * Basic definition of a single inode in the VFS. Contains file data and
 * filesystem-level operations. Name and hierarchy are managed by dentry_t.
 * TODO: Implementation of ownership and permissions system (owner, group,
 * read/write/execute flags)
 */
typedef struct vfs_node {
    char filename[VFS_MAX_FILENAME]; // Filesystem-internal name (used by filesystem callbacks)
    uint8_t type; // one of VFS_TYPE_
    uint32_t inode; // File ID, device specific, to identify files (on a disk)
    uint64_t length; // File size in bytes
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
    ioctl_node_t ioctl_node;
    sync_node_t sync_node;
    void* metadata; // Data that can be used by filesystem
} vfs_node_t;

// One of these is returned by the readdir call, according to POSIX.
struct dirent
{
  char name[VFS_MAX_FILENAME]; // Filename.
  uint32_t ino; // Inode number. Required by POSIX.
};

// The root dentry of the filesystem
extern dentry_t* vfs_root;

size_t vfs_read(vfs_file_t* file, size_t size, void* ptr);
void vfs_seek(vfs_file_t* file, uint64_t offset);
size_t vfs_write(vfs_file_t* file, size_t size, const void* ptr);
vfs_file_t* vfs_open(dentry_t* dentry, uint8_t open_mode);
void vfs_close(vfs_file_t* file);
bool vfs_readdir(dentry_t* dentry, size_t index, struct dirent* out);
dentry_t* vfs_finddir(dentry_t* dentry, const char* name);
void vfs_sync(const vfs_file_t* file);

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
 * Resolves an absolute path and returns a valid dentry, or NULL if not found.
 */
dentry_t* vfs_resolve(const char* abs_path);

/**
 * Resolves the passed path, either as an absolute path (when starting with /)
 * or a path relative to [current], while making sure that it doesn't go outside
 * of [root] dentry.
 */
dentry_t* vfs_resolve_relative(
    dentry_t* root,
    dentry_t* current,
    const char* path
);

/**
 * Builds an absolute path of [current] dentry, starting from [root], stores it
 * as a null-terminated string in [buff], and returns pointer to [buff]. If the
 * path, including null byte, is longer than [size], then it returns null.
 */
char* vfs_build_absolute_path(
    dentry_t* root,
    dentry_t* current,
    char* buff,
    size_t size
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
 * Returns the new dentry, or NULL if `parent` is NULL or not a directory.
 */
dentry_t* vfs_mkdir(dentry_t* parent, const char* name);

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

#define MOUNT_FILESYSTEM_ISO9660 "iso9660"
#define MOUNT_FILESYSTEM_EXT2 "ext2"

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

#endif
