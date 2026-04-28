#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/memory/kmalloc.h"
#include "./ext2.h"

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2/finddir", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2/finddir", __VA_ARGS__)

vfs_node_t* ext2_finddir(vfs_node_t* node, char* name)
{
    ext2_inode_metadata_t* meta = node->metadata;

    vfs_file_t* file = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (file == NULL) {
        _debug_puts("result of vfs_open is NULL");
        return NULL;
    }

    ext2_inode_t* dir_inode = ext2_read_inode(file, meta, node->inode);

    uint32_t bs = meta->block_size;
    uint8_t* block_buf = kmalloc(bs);
    uint32_t found_inode_num = 0;
    size_t name_len = strlen(name);

    for (int i = 0; i < 12 && found_inode_num == 0; i++) {
        uint32_t block_ptr = dir_inode->direct_block_pointers[i];
        if (block_ptr == 0) break;

        vfs_seek(file, block_ptr * bs);
        vfs_read(file, bs, block_buf);

        uint32_t offset = 0;
        while (offset < bs) {
            ext2_dir_entry_t* entry = (ext2_dir_entry_t*)(block_buf + offset);
            if (entry->rec_len == 0) break;

            if (entry->inode != 0
                && entry->name_len == name_len
                && memcmp(entry->name, name, name_len) == 0) {
                found_inode_num = entry->inode;
                break;
            }
            offset += entry->rec_len;
        }
    }

    kfree(block_buf);
    kfree(dir_inode);

    if (found_inode_num == 0) {
        vfs_close(file);
        return NULL;
    }

    ext2_inode_t* child_ext2_inode =
        ext2_read_inode(file, meta, found_inode_num);
    vfs_close(file);

    vfs_node_t* result = kmalloc(sizeof(vfs_node_t));
    uint8_t vfs_type = ext2_type_to_vfs(child_ext2_inode->type_and_permissions);
    vfs_populate_node(result, name, vfs_type);
    result->inode = found_inode_num;
    result->length = child_ext2_inode->size_lo;
    result->on_release = ext2_on_release;

    ext2_inode_metadata_t* child_meta = kmalloc(sizeof(ext2_inode_metadata_t));
    *child_meta = *meta;
    result->metadata = child_meta;

    if (vfs_type == VFS_TYPE_DIRECTORY) {
        result->readdir_node = ext2_readdir;
        result->finddir_node = ext2_finddir;
    }

    kfree(child_ext2_inode);
    return result;
}
