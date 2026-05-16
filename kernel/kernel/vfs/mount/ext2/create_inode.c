#include <stdbool.h>
#include <string.h>
#include <time.h>
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
    vfs_node_t* parent,
    ext2_type_t type
) {
    ext2_inode_metadata_t* meta = parent->metadata;

    uint32_t timestamp = (uint32_t)time(NULL);

    // TODO: using default permissions here. Maybe inherit them from parent?
    inode->type_and_permissions = type | 0755;
    // TODO: set valid UID and GID
    inode->user_id = meta->cached_inode->user_id;
    inode->group_id = meta->cached_inode->group_id;
    inode->creation_time = timestamp;
    inode->last_access_time = timestamp;
    inode->last_modification_time = timestamp;

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

uint32_t ext2_create_inode(
    vfs_node_t* parent_inode,
    const char* name,
    ext2_type_t type,
    ext2_inode_t* result
) {
    ext2_inode_metadata_t* meta = parent_inode->metadata;

    _debug_printf(
        "Run create_inode with name '%s' on inode %u ('%s')\n",
        name,
        parent_inode->inode,
        parent_inode->filename
    );

    if ((parent_inode->type & VFS_TYPE_DIRECTORY) == 0) {
        _debug_puts("Parent node is not a directory");
        return 0;
    }

    if (check_if_exists(parent_inode, name)) {
        _debug_puts("Parent node already contains a node with given name");
        return 0;
    }

    // Allocate inode for the new directory
    uint32_t ino = ext2_alloc_inode(meta->device_file, meta->block_size);
    if (ino == 0) {
        _debug_puts("Failed to allocate inode");
        return 0;
    }

    // Create inode for new file, populate metadata, and save to disk
    memset(result, 0, sizeof(ext2_inode_t));
    populate_inode_metadata(result, parent_inode, type);
    ext2_write_inode(
        meta->device_file,
        ino,
        meta->block_size,
        meta->inode_size,
        meta->inodes_per_group,
        result
    );

    // Add new entry to the parent
    if (!ext2_add_inode_to_dir(meta, parent_inode->inode, ino, name)) {
        _debug_puts("Failed to add child to parent directory");
        return 0;
        // TODO: cleanup - dealloc inode
    }

    // Invalidate parent node's in-memory caches so the next readdir/finddir
    // reloads from disk and sees the new entry.
    if (meta->dir_cache != NULL) {
        kfree(meta->dir_cache);
        meta->dir_cache = NULL;
        meta->dir_cache_size = 0;
    }
    if (meta->cached_inode != NULL) {
        kfree(meta->cached_inode);
        meta->cached_inode = NULL;
    }

    return ino;
}
