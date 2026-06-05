#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/memory/kmalloc.h"
#include "./ext2.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_EXT2, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_EXT2, __VA_ARGS__)

vfs_node_t* ext2_finddir(vfs_node_t* node, char* name)
{
    ext2_inode_metadata_t* meta = node->metadata;

    if (!ext2_ensure_dir_cache(node)) {
        return NULL;
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

    size_t name_len = strlen(name);
    uint32_t found_inode_num = 0;

    uint32_t offset = 0;
    while (offset < meta->dir_cache_size) {
        ext2_dir_entry_t* entry = (ext2_dir_entry_t*)(meta->dir_cache + offset);
        if (entry->rec_len == 0) {
            break;
        }

        if (entry->inode != 0
            && entry->name_len == name_len
            && memcmp(entry->name, name, name_len) == 0) {
            found_inode_num = entry->inode;
            break;
        }
        offset += entry->rec_len;
    }

    if (found_inode_num == 0) return NULL;

    vfs_file_t* file = vfs_open(meta->device_file, O_RDONLY, NULL);
    if (file == NULL) {
        _debug_puts("result of vfs_open is NULL");
        return NULL;
    }

    ext2_inode_t* child_ext2_inode =
        ext2_read_inode(file, meta, found_inode_num);
    vfs_close(file);

    vfs_node_t* result = kmalloc(sizeof(vfs_node_t));
    uint8_t vfs_type = ext2_type_to_vfs(child_ext2_inode->type_and_permissions);
    vfs_populate_node(result, name, vfs_type);
    result->inode = found_inode_num;
    result->permissions = ext2_mode_to_perm(
        child_ext2_inode->type_and_permissions
    );
    result->length = child_ext2_inode->size_lo;
    result->on_release = ext2_on_release;
    result->stat_node = ext2_stat;

    ext2_inode_metadata_t* child_meta = kmalloc_flags(
        sizeof(ext2_inode_metadata_t),
        KMALLOC_ZERO
    );
    *child_meta = *meta;
    child_meta->cached_inode = NULL;
    child_meta->dir_cache = NULL;
    child_meta->dir_cache_size = 0;
    result->metadata = child_meta;

    if (vfs_type == VFS_TYPE_DIRECTORY) {
        result->readdir_node = ext2_readdir;
        result->finddir_node = ext2_finddir;
        result->mkdir_node = ext2_mkdir;
        result->mkfile_node = ext2_mkfile;
    } else if (vfs_type == VFS_TYPE_SYMLINK) {
        result->readlink_node = ext2_readlink;
    } else {
        result->open_node = ext2_open;
        result->close_node = ext2_close;
        result->read_node = ext2_read;
        result->write_node = ext2_write;
        result->truncate_node = ext2_truncate;
    }

    kfree(child_ext2_inode);
    return result;
}
