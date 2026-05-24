#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "dirent.h"
#include "kernel/vfs/vfs.h"
#include "./ext2.h"

bool ext2_readdir(
    vfs_node_t* node,
    size_t index,
    struct dirent* out
) {
    ext2_inode_metadata_t* meta = node->metadata;

    if (!ext2_ensure_dir_cache(node)) {
        return false;
    }
    if (meta->dir_cache_size == 0) {
        return false;
    }

    // Update directory's last access time
    meta->cached_inode->last_access_time = (uint32_t)time(NULL);
    ext2_write_inode(
        meta->device_file,
        node->inode,
        meta->block_size,
        meta->inode_size,
        meta->inodes_per_group,
        meta->cached_inode
    );

    size_t count = 0;
    uint32_t offset = 0;
    while (offset < meta->dir_cache_size) {
        ext2_dir_entry_t* entry = (ext2_dir_entry_t*)(meta->dir_cache + offset);
        if (entry->rec_len == 0) break;

        if (entry->inode != 0) {
            if (count == index) {
                out->d_ino = entry->inode;
                size_t nl = entry->name_len < VFS_MAX_FILENAME - 1
                    ? entry->name_len
                    : VFS_MAX_FILENAME - 1;
                memcpy(out->d_name, entry->name, nl);
                out->d_name[nl] = '\0';

                vfs_file_t* file = vfs_open(meta->device_file, VFS_MODE_READONLY);
                if (file == NULL) {
                    return NULL;
                }

                ext2_inode_t* child_ext2_inode =
                    ext2_read_inode(file, meta, entry->inode);
                vfs_close(file);
                uint8_t vfs_type = ext2_type_to_vfs(
                    child_ext2_inode->type_and_permissions
                );
                out->d_type = vfs_type_to_dirent(vfs_type);
                return true;
            }
            count++;
        }
        offset += entry->rec_len;
    }
    return false;
}
