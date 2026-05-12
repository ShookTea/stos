#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "./ext2.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_EXT2, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_EXT2, __VA_ARGS__)

static uint8_t dir_entry_file_type(uint8_t vfs_type)
{
    switch (vfs_type) {
        case VFS_TYPE_FILE: return 1;
        case VFS_TYPE_DIRECTORY: return 2;
        case VFS_TYPE_SYMLINK: return 7;
        case VFS_TYPE_CHARACTER_DEVICE: return 3;
        case VFS_TYPE_BLOCK_DEVICE: return 4;
        default: return 0;
    }
}

inline uint16_t pad_len(uint16_t len)
{
    return (len + 3) & ~3u;
}

bool ext2_add_inode_to_dir(
    ext2_inode_metadata_t* meta,
    uint32_t parent_inode_num,
    uint32_t child_inode_num,
    const char* name
) {
    _debug_printf("Adding entry %u to parent %u with name '%s'\n", child_inode_num, parent_inode_num, name);
    vfs_file_t* device_file = vfs_open(meta->device_file, VFS_MODE_READWRITE);
    if (device_file == NULL) {
        return false;
    }
    ext2_inode_t* parent_inode = ext2_read_inode(
        device_file,
        meta,
        parent_inode_num
    );
    if (parent_inode == NULL) {
        vfs_close(device_file);
        return false;
    }
    uint8_t parent_type = ext2_type_to_vfs(parent_inode->type_and_permissions);
    if ((parent_type & VFS_TYPE_DIRECTORY) == 0) {
        kfree(parent_inode);
        vfs_close(device_file);
        return false;
    }

    ext2_inode_t* child_inode = ext2_read_inode(
        device_file,
        meta,
        child_inode_num
    );
    if (child_inode == NULL) {
        kfree(parent_inode);
        vfs_close(device_file);
        return false;
    }

    // Create dir entry
    uint8_t name_length = strlen(name);
    // 8 bytes for header; records must be aligned to 4-byte boundaries
    uint8_t rec_length = pad_len(name_length) + 8; // 8 header bytes
    ext2_dir_entry_t* dir_entry = kmalloc_flags(
        rec_length,
        KMALLOC_ZERO
    );
    uint8_t child_type = ext2_type_to_vfs(child_inode->type_and_permissions);

    dir_entry->inode = child_inode_num;
    dir_entry->rec_len = rec_length;
    dir_entry->name_len = name_length;
    dir_entry->file_type = dir_entry_file_type(child_type);
    memcpy(dir_entry->name, name, name_length);

    // Iterate over blocks in the directory, to find an empty spot for a new
    // dir entry. Because empty space is not allowed between entries, there
    // are three possibilities:
    // 1. dir entry has a much longer rec_len and has a lot of empty space
    //    inside that can be used
    // 2. dir entry has an inode=0, marking it as unused
    // 3. we've reached the end of dir entries

    size_t current_block_index = 0;
    bool success = false;
    uint8_t* buffer = kmalloc_flags(meta->block_size, KMALLOC_ZERO);
    uint8_t* zeros = NULL;

    while (true) {
        bool new_block_allocated = false;
        size_t block_addr = ext2_block_id_to_addr(
            device_file->dentry,
            parent_inode,
            current_block_index,
            meta->block_size
        );
        if (block_addr == 0) {
            // New block needs to be allocated
            uint32_t new_block = ext2_alloc_block(
                device_file->dentry,
                meta->block_size
            );
            if (new_block == 0) {
                _debug_puts("ext2_alloc_block failed - disk full?");
                break;
            }
            bool set_block_id_res = ext2_set_block_id(
                device_file->dentry,
                parent_inode,
                current_block_index,
                meta->block_size,
                new_block
            );
            if (!set_block_id_res) {
                _debug_puts("ext2_set_block_id failed");
                break;
            }
            if (zeros == NULL) {
                zeros = kmalloc_flags(meta->block_size, KMALLOC_ZERO);
            }
            vfs_seek(device_file, (uint64_t)new_block * meta->block_size);
            vfs_write(device_file, meta->block_size, zeros);
            block_addr = (size_t)new_block * meta->block_size;
            parent_inode->size_lo += meta->block_size;
            new_block_allocated = true;
        }

        vfs_seek(device_file, block_addr);
        vfs_read(device_file, meta->block_size, buffer);

        if (new_block_allocated) {
            // We created a brand new block for this inode anyway, so we can
            // just save that inode there. But first we need to update rec_len,
            // to use full block size.
            memset(buffer, 0, meta->block_size);
            ext2_dir_entry_t* de = (ext2_dir_entry_t*)buffer;
            de->inode = child_inode_num;
            de->rec_len = meta->block_size;
            de->name_len = name_length;
            de->file_type = dir_entry_file_type(child_type);
            memcpy(de->name, name, name_length);

            vfs_seek(device_file, block_addr);
            vfs_write(device_file, meta->block_size, buffer);
            success = true;
            _debug_printf("Written entry to newly allocated block at addr %u\n", block_addr);
            break;
        }

        // Iterate over dir entries
        size_t bytes_read = 0;
        while (bytes_read < meta->block_size) {
            ext2_dir_entry_t* read_entry =
                (ext2_dir_entry_t*)(buffer + bytes_read);
            _debug_printf(
                "Block=%u byte=%u inode=%u rec_len=%u\n",
                current_block_index,
                bytes_read,
                read_entry->inode,
                read_entry->rec_len
            );
            if (read_entry->inode == 0) {
                if (read_entry->rec_len >= rec_length) {
                    // This entry can be used. Let's replace it, but keep the
                    // original rec_len.
                    uint16_t rec_len_original = read_entry->rec_len;
                    memcpy(read_entry, dir_entry, rec_length);
                    read_entry->rec_len = rec_len_original;
                    vfs_seek(device_file, block_addr);
                    vfs_write(device_file, meta->block_size, buffer);
                    success = true;
                    _debug_printf(
                        "Replacing inode=0 with this node\n"
                    );
                    break;
                }
                else if (read_entry->rec_len == 0) {
                    _debug_puts("Corrupted directory (rec_len == 0)");
                    // Break inner loop
                    break;
                } else {
                    // Shift to the next position
                    bytes_read += read_entry->rec_len;
                }
            } else {
                // Check if rec_len of the entry can easily fit more data.
                uint16_t real_len = 8 + pad_len(read_entry->name_len);
                uint16_t slack_space = read_entry->rec_len - real_len;
                if (slack_space >= rec_length) {
                    _debug_printf(
                        "Cutting %u bytes of slack space - updating rec_len to %u\n",
                        slack_space,
                        real_len
                    );
                    // There's enough space. Cut it's length to the actual size:
                    read_entry->rec_len = real_len;
                    // Shift to the position of next entry
                    bytes_read += real_len;
                    // Insert new node here, but use slack_space for rec_len
                    // instead of actual rec_len
                    dir_entry->rec_len = slack_space;
                    memcpy(buffer + bytes_read, dir_entry, rec_length);
                    vfs_seek(device_file, block_addr);
                    vfs_write(device_file, meta->block_size, buffer);
                    success = true;
                    break;
                } else {
                    // There's not enough space in this entry - let's move to
                    // the next one.
                    bytes_read += read_entry->rec_len;
                }
            }
        }

        if (success) {
            break;
        }

        // No valid entry found in this block - move to the next one
        current_block_index++;
    }

    if (success) {
        // Update child's hard link count
        child_inode->hard_links_count++;
        // Save data
        ext2_write_inode(
            device_file->dentry,
            parent_inode_num,
            meta->block_size,
            meta->inode_size,
            meta->inodes_per_group,
            parent_inode
        );
        ext2_write_inode(
            device_file->dentry,
            child_inode_num,
            meta->block_size,
            meta->inode_size,
            meta->inodes_per_group,
            child_inode
        );
    }

    kfree(parent_inode);
    kfree(child_inode);
    kfree(dir_entry);
    kfree(buffer);
    if (zeros != NULL) {
        kfree(zeros);
    }
    vfs_close(device_file);
    return success;
}
