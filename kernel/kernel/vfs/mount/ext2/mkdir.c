#include <stdbool.h>
#include <string.h>
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "./ext2.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_EXT2, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_EXT2, __VA_ARGS__)

/**
 * Returns `true` if there's already a node with given `name` in `node`.
 */
static bool check_if_exists(vfs_node_t* node, const char* name)
{
    // Try to verify if given name already exists
    char* name2 = kmalloc(sizeof(char) * strlen(name) + 1);
    strcpy(name2, name);
    vfs_node_t* existing = ext2_finddir(node, name2);
    kfree(name2);

    if (existing != NULL) {
        existing->on_release(existing);
        return true;
    }

    return false;
}

static void populate_inode_metadata(
    ext2_inode_t* inode,
    vfs_node_t* parent
) {
    ext2_inode_metadata_t* meta = parent->metadata;

    // TODO: using default permissions here. Maybe inherit them from parent?
    inode->type_and_permissions = 0x4000 | 0755;
    // TODO: set valid UID and GID
    inode->user_id = meta->cached_inode->user_id;
    inode->group_id = meta->cached_inode->group_id;
    // TODO: set valid timestamps
    inode->creation_time = 0;
    inode->last_access_time = 0;
    inode->last_modification_time = 0;

    inode->size_lo = 0;
    inode->hard_links_count = 0;

    // Zero all block pointers
    memset(
        inode->direct_block_pointers,
        0,
        sizeof(inode->direct_block_pointers)
    );
    inode->singly_indirect_block_pointer = 0;
    inode->doubly_indirect_block_pointer = 0;
    inode->triply_indirect_block_pointer = 0;
}

bool ext2_mkdir(vfs_node_t* node, const char* name)
{
    ext2_inode_metadata_t* meta = node->metadata;

    _debug_printf(
        "Run mkdir with name '%s' on inode %u ('%s')\n",
        name,
        node->inode,
        node->filename
    );

    if ((node->type & VFS_TYPE_DIRECTORY) == 0) {
        _debug_puts("Parent node is not a directory");
        return false;
    }

    if (check_if_exists(node, name)) {
        _debug_puts("Parent node already contains a node with given name");
        return false;
    }

    // Allocate inode for the new directory
    uint32_t ino = ext2_alloc_inode(meta->device_file, meta->block_size);
    if (ino == 0) {
        _debug_puts("Failed to allocate inode");
        return false;
    }

    // Create inode for new directory, populate metadata, and save to disk
    ext2_inode_t new_inode = {0};
    populate_inode_metadata(&new_inode, node);
    ext2_write_inode(
        meta->device_file,
        ino,
        meta->block_size,
        meta->inode_size,
        meta->inodes_per_group,
        &new_inode
    );

    // Add "." and ".." to the new directory's content, and add new directory
    // to the parent.
    if (!ext2_add_inode_to_dir(meta, ino, ino, ".")) {
        _debug_puts("Failed to create '.' entry");
        return false;
        // TODO: cleanup - dealloc inode
    }
    if (!ext2_add_inode_to_dir(meta, ino, node->inode, "..")) {
        _debug_puts("Failed to create '..' entry");
        return false;
        // TODO: cleanup - remove "." and dealloc inode
    }
    if (!ext2_add_inode_to_dir(meta, node->inode, ino, name)) {
        _debug_puts("Failed to add child to parent directory");
        return false;
        // TODO: cleanup - remove "." and "..", and dealloc inode
    }

    return true;
}
