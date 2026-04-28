#include <stdint.h>
#include "kernel/vfs/vfs.h"
#include "./ext2.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2/close", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2/close", __VA_ARGS__)

void ext2_close(vfs_node_t* node, vfs_file_t* file)
{
    _debug_printf(
        "Closing inode %u '%s'\n",
        node->inode,
        file->dentry->name
    );

    if (file->metadata == NULL) {
        return;
    }

    ext2_file_metadata_t* meta = file->metadata;
    if (meta->cached_inode != NULL) {
        kfree(meta->cached_inode);
    }
    kfree(meta);
    file->metadata = NULL;
}
