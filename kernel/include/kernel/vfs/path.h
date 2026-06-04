#ifndef INCLUDE_KERNEL_VFS_PATH_H
#define INCLUDE_KERNEL_VFS_PATH_H

#include "kernel/vfs/vfs.h"

/**
 * Resolves absolute path to a file, from the VFS root. Returns NULL if path
 * doesn't exist.
 */
dentry_t* path_resolve_absolute(const char* abs_path);

/**
 * Resolves given path as relative to `current` path, using `root` as root
 * directory. Returns NULL if path doesn't exist.
 */
dentry_t* path_resolve_relative(
    dentry_t* root,
    dentry_t* current,
    const char* path
);

/**
 * Like path_resolve_relative, but does NOT follow a symlink that is the final
 * component of the path. Returns the symlink dentry itself. Used by readlink()
 * and lstat().
 */
dentry_t* path_resolve_relative_no_follow(
    dentry_t* root,
    dentry_t* current,
    const char* path
);

/**
 * Resolves given path relative to `current`. If file exists, returns dentry.
 * Otherwise returns parent of that entry, if exists. If parent also doesn't
 * exist, returns NULL.
 *
 * If `basename` is not NULL, it will be allocated with the name of the file
 * pointed to by the path. That has to be freed later.
 *
 * If `path_exists` is not NULL, it will be set to `true` if the file exists
 * under given path.
 */
dentry_t* path_resolve_optional(
    dentry_t* root,
    dentry_t* current,
    const char* path,
    char** basename,
    bool* path_exists
);

/**
 * Builds an absolute path of `current` dentry, starting from `root`, stores it
 * as a null-terminated string in `buff`, and returns pointer to `buff`. If the
 * path, including null byte, is longer than `size`, then it returns null.
 */
char* path_build_absolute(
    dentry_t* root,
    dentry_t* current,
    char* buff,
    size_t size
);

#endif
