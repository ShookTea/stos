#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
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
                out->ino = entry->inode;
                size_t nl = entry->name_len < VFS_MAX_FILENAME - 1
                    ? entry->name_len
                    : VFS_MAX_FILENAME - 1;
                memcpy(out->name, entry->name, nl);
                out->name[nl] = '\0';
                return true;
            }
            count++;
        }
        offset += entry->rec_len;
    }
    return false;
}
