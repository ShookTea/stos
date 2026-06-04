#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <stdio.h>
#include <string.h>

#define VFS_SYMLINK_MAX_DEPTH 40

/**
 * Shared implementation for vfs_resolve and vfs_resolve_relative.
 * symlink_depth tracks followed symlinks to detect loops.
 * follow_last_symlink controls whether a symlink that is the final path
 * component gets followed (false = return the symlink dentry itself, as
 * needed by readlink and lstat).
 *
 * If `allow_nonexisting_last_entry` is `true`, the final file pointed to by
 * the path may not exist. If that's the case, then this function will return
 * a `dentry_t` of the parent directory of that file, and `last_entry_not_exist`
 * (if not NULL) will be set to `true`.
 *
 * At the end of successful resolving, including if `allow_nonexisting_last_entry`
 * was `true` and final file didn't exist, if `basename` is not a null pointer,
 * it will be populated with the target filename. If `allow_nonexisting_last_entry`
 * was `true` and file didn't exist, it will be populated with the name of that
 * file (as opposed to the name of parent directory that is returned as dentry).
 * The result will have to be deallocated.
 */
static dentry_t* vfs_resolve_impl(
    dentry_t* root,
    dentry_t* start,
    const char* path,
    int symlink_depth,
    bool follow_last_symlink,
    bool allow_nonexisting_last_entry,
    bool* last_entry_not_exists,
    char** basename
) {
    if (symlink_depth >= VFS_SYMLINK_MAX_DEPTH) {
        return NULL;
    }
    if (last_entry_not_exists != NULL) {
        *last_entry_not_exists = false;
    }
    if (path == NULL || path[0] == '\0') {
        if (basename != NULL && start != NULL) {
            *basename = kmalloc(strlen(start->name) + 1);
            strcpy(*basename, start->name);
        }
        return start;
    }

    dentry_t* node = (path[0] == '/') ? root : start;
    char* captured_basename = NULL;

    size_t path_len = strlen(path);
    char* path_copy = kmalloc(path_len + 1);
    strcpy(path_copy, path);

    char* ptr = path_copy;
    while (*ptr != '\0' && node != NULL) {
        // Skip slashes
        while (*ptr == '/') ptr++;
        if (*ptr == '\0') break;

        // Isolate the next component
        char* end = ptr;
        while (*end != '\0' && *end != '/') end++;
        char saved = *end;
        *end = '\0';

        if (strcmp(ptr, ".") == 0) {
            // Stay in current directory
        } else if (strcmp(ptr, "..") == 0) {
            if (node != root) {
                node = node->parent;
                if (node == NULL) {
                    node = root; // Safety: clamp if tree is malformed
                }
            }
        } else {
            dentry_t* parent_dir = node;
            node = vfs_finddir(node, ptr);

            if (node == NULL && allow_nonexisting_last_entry) {
                bool is_last = (saved == '\0');
                if (!is_last) {
                    char* rest = end + 1;
                    while (*rest == '/') rest++;
                    is_last = (*rest == '\0');
                }
                if (is_last) {
                    if (last_entry_not_exists != NULL) {
                        *last_entry_not_exists = true;
                    }
                    captured_basename = ptr;
                    node = parent_dir;
                    break;
                }
            }

            if (node != NULL && node->inode->type == VFS_TYPE_SYMLINK
                && (follow_last_symlink || saved != '\0')
            ) {
                if (node->inode->readlink_node == NULL) {
                    node = NULL;
                    break;
                }

                char* target = kmalloc(VFS_MAX_PATH_LENGTH);
                int r = node->inode->readlink_node(
                    node->inode, target, VFS_MAX_PATH_LENGTH - 1
                );
                if (r < 0) {
                    kfree(target);
                    node = NULL;
                    break;
                }
                target[r] = '\0';

                // Restore saved so 'end' shows the remaining path
                // ('\0' or '/...')
                *end = saved;

                char* combined = kmalloc(VFS_MAX_PATH_LENGTH);
                if (saved == '\0') {
                    // Symlink is the final component — no remaining path
                    strncpy(combined, target, VFS_MAX_PATH_LENGTH - 1);
                    combined[VFS_MAX_PATH_LENGTH - 1] = '\0';
                } else {
                    // 'end' points to '/' followed by the remaining components
                    snprintf(
                        combined,
                        VFS_MAX_PATH_LENGTH,
                        "%s%s",
                        target,
                        end
                    );
                }

                dentry_t* base = (combined[0] == '/') ? root : parent_dir;
                kfree(path_copy);
                kfree(target);
                dentry_t* result = vfs_resolve_impl(
                    root,
                    base,
                    combined,
                    symlink_depth + 1,
                    follow_last_symlink,
                    allow_nonexisting_last_entry,
                    last_entry_not_exists,
                    basename
                );
                kfree(combined);
                return result;
            }
        }

        *end = saved;
        ptr = end;
    }

    if (basename != NULL) {
        if (captured_basename == NULL) {
            captured_basename = node != NULL ? node->name : NULL;
        }
        if (captured_basename != NULL) {
            *basename = kmalloc(strlen(captured_basename) + 1);
            strcpy(*basename, captured_basename);
        }
    }

    kfree(path_copy);
    return node;
}

dentry_t* vfs_resolve(const char* abs_path)
{
    return vfs_resolve_impl(
        vfs_root,
        vfs_root,
        abs_path,
        0,
        true,
        false,
        NULL,
        NULL
    );
}

dentry_t* vfs_resolve_relative(
    dentry_t* root,
    dentry_t* current,
    const char* path
) {
    if (path == NULL || path[0] == '\0') {
        return current;
    }
    return vfs_resolve_impl(root, current, path, 0, true, false, NULL, NULL);
}

dentry_t* vfs_resolve_relative_no_follow(
    dentry_t* root,
    dentry_t* current,
    const char* path
) {
    if (path == NULL || path[0] == '\0') {
        return current;
    }
    return vfs_resolve_impl(root, current, path, 0, false, false, NULL, NULL);
}

char* vfs_build_absolute_path(
    dentry_t* root,
    dentry_t* current,
    char* buff,
    size_t size
) {
    char** parts = NULL;
    size_t parts_count = 0;
    size_t total_length = 0; // excluding file separators
    while (current != root) {
        parts_count++;
        total_length += strlen(current->name);
        parts = krealloc(parts, sizeof(char*) * parts_count);
        parts[parts_count - 1] = current->name;
        current = current->parent;
    }
    size_t sep_count = parts_count == 0 ? 1 : parts_count;
    if ((sep_count + total_length + 1) > size) {
        if (parts != NULL) kfree(parts);
        return NULL;
    }

    if (parts_count == 0) {
        buff[0] = '/';
        buff[1] = 0;
        // No need for kfree if parts_count == 0
        return buff;
    }

    size_t buff_index = 0;
    for (int i = parts_count - 1; i >= 0; i--) {
        buff[buff_index] = '/';
        buff_index++;
        strcpy(buff + buff_index, parts[i]);
        buff_index += strlen(parts[i]);
    }
    kfree(parts);
    return buff;
}
