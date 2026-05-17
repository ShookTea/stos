#include "./ext2.h"
#include "kernel/vfs/vfs.h"
#include <errno.h>

int ext2_stat(const struct vfs_node* node, struct stat* stat)
{
    if (node == NULL) {
        return EBADF;
    }
    if (stat == NULL) {
        return ENOMEM;
    }

    ext2_inode_metadata_t* meta = node->metadata;
    ext2_inode_t* inode = meta->cached_inode;
    stat->st_dev = 0; // TODO: set this when devices are correctly implemented
    stat->st_ino = node->inode;
    stat->st_mode = inode->type_and_permissions;
    stat->st_nlink = inode->hard_links_count;
    stat->st_uid = inode->user_id;
    stat->st_gid = inode->group_id;

    uint8_t vfs_type = ext2_type_to_vfs(inode->type_and_permissions);
    if (vfs_type == VFS_TYPE_BLOCK_DEVICE
        || vfs_type == VFS_TYPE_CHARACTER_DEVICE
    ) {
        stat->st_rdev = inode->direct_block_pointers[0];
    }
    stat->st_size = node->length;
    stat->st_blksize = meta->block_size;
    stat->st_blocks = 0; // TODO: what should actually be here?
    stat->st_atim.tv_spec = inode->last_access_time;
    stat->st_mtim.tv_spec = inode->last_modification_time;
    stat->st_ctim.tv_spec = inode->creation_time;
    stat->st_atim.tv_nsec = 0;
    stat->st_mtim.tv_nsec = 0;
    stat->st_ctim.tv_nsec = 0;

    return 0;
}
