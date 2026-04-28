#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/debug.h"
#include "./ext2.h"


#define _debug_puts(...) debug_puts_c("VFS/mount/ext2/readdir", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2/readdir", __VA_ARGS__)

bool ext2_readdir(
    vfs_node_t* node,
    size_t index,
    struct dirent* out
) {
    ext2_inode_metadata_t* meta = node->metadata;

    vfs_file_t* file = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (file == NULL) {
        _debug_puts("result of vfs_open is NULL");
        return false;
    }

    ext2_inode_t* dir_inode = ext2_read_inode(file, meta, node->inode);

    uint32_t bs = meta->block_size;
    uint8_t* block_buf = kmalloc(bs);
    size_t count = 0;
    bool found = false;

    for (int i = 0; i < 12 && !found; i++) {
        uint32_t block_ptr = dir_inode->direct_block_pointers[i];
        if (block_ptr == 0) break;

        _debug_printf(
            "readdir: reading data block %u at byte 0x%X\n",
            block_ptr,
            block_ptr * bs
        );
        vfs_seek(file, block_ptr * bs);
        vfs_read(file, bs, block_buf);

        uint32_t offset = 0;
        while (offset < bs && !found) {
            ext2_dir_entry_t* entry = (ext2_dir_entry_t*)(block_buf + offset);
            if (entry->rec_len == 0) break;

            _debug_printf(
                "readdir: entry inode=%u rec_len=%u name_len=%u\n",
                entry->inode, entry->rec_len, entry->name_len
            );

            if (entry->inode != 0) {
                if (count == index) {
                    out->ino = entry->inode;
                    size_t nl = entry->name_len < VFS_MAX_FILENAME - 1
                        ? entry->name_len
                        : VFS_MAX_FILENAME - 1;
                    memcpy(out->name, entry->name, nl);
                    out->name[nl] = '\0';
                    found = true;
                } else {
                    count++;
                }
            }
            offset += entry->rec_len;
        }
    }

    kfree(block_buf);
    kfree(dir_inode);
    vfs_close(file);
    return found;
}
