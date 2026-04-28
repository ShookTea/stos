#include "ext2.h"
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/debug.h"
#include <stdint.h>
#include <string.h>
#include <math.h>

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2", __VA_ARGS__)

static bool ext2_readdir(struct vfs_node* node, size_t index, struct dirent* out);
static struct vfs_node* ext2_finddir(struct vfs_node* node, char* name);

static void ext2_on_release(vfs_node_t* node)
{
    if (node == NULL) {
        return;
    }
    if (node->metadata != NULL) {
        kfree(node->metadata);
    }
    kfree(node);
}

static ext2_inode_t* read_ext2_inode(
    vfs_file_t* file,
    ext2_inode_metadata_t* meta,
    uint32_t inode_num
) {
    uint32_t bs = meta->block_size;
    uint32_t block_group = (inode_num - 1) / meta->inodes_per_group;
    uint32_t index_in_group = (inode_num - 1) % meta->inodes_per_group;

    uint32_t gdt_start = (bs == 1024 ? 2 : 1) * bs;
    uint32_t gdt_seek = gdt_start + block_group * sizeof(ext2_gdt_t);
    vfs_seek(file, gdt_seek);
    ext2_gdt_t* gdt = kmalloc_flags(sizeof(ext2_gdt_t), KMALLOC_ZERO);
    vfs_read(file, sizeof(ext2_gdt_t), gdt);
    uint32_t inode_table_byte = gdt->inode_table_start_addr * bs;
    kfree(gdt);

    uint32_t inode_byte = inode_table_byte + index_in_group * meta->inode_size;
    ext2_inode_t* inode = kmalloc_flags(sizeof(ext2_inode_t), KMALLOC_ZERO);
    vfs_seek(file, inode_byte);
    vfs_read(file, sizeof(ext2_inode_t), inode);
    return inode;
}

static uint8_t ext2_type_to_vfs(uint16_t type_and_permissions)
{
    switch (type_and_permissions & 0xF000) {
        case 0x4000: return VFS_TYPE_DIRECTORY;
        case 0x8000: return VFS_TYPE_FILE;
        case 0xA000: return VFS_TYPE_SYMLINK;
        case 0x6000: return VFS_TYPE_BLOCK_DEVICE;
        case 0x2000: return VFS_TYPE_CHARACTER_DEVICE;
        default:     return VFS_TYPE_FILE;
    }
}

static bool ext2_readdir(
    struct vfs_node* node,
    size_t index,
    struct dirent* out
) {
    ext2_inode_metadata_t* meta = node->metadata;

    vfs_file_t* file = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (file == NULL) {
        _debug_puts("result of vfs_open is NULL");
        return false;
    }

    ext2_inode_t* dir_inode = read_ext2_inode(file, meta, node->inode);

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

static struct vfs_node* ext2_finddir(struct vfs_node* node, char* name)
{
    ext2_inode_metadata_t* meta = node->metadata;

    vfs_file_t* file = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (file == NULL) {
        _debug_puts("result of vfs_open is NULL");
        return NULL;
    }

    ext2_inode_t* dir_inode = read_ext2_inode(file, meta, node->inode);

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
        read_ext2_inode(file, meta, found_inode_num);
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

vfs_mount_result_t vfs_mount_ext2(
    dentry_t* device_file,
    dentry_t* target,
    uint16_t flags __attribute__((unused)),
    const void* data __attribute__((unused))
) {
    char* device_path = kmalloc_flags(VFS_MAX_PATH_LENGTH, KMALLOC_ZERO);
    char* target_path = kmalloc_flags(VFS_MAX_PATH_LENGTH, KMALLOC_ZERO);
    vfs_build_absolute_path(
        vfs_get_real_root(),
        device_file,
        device_path,
        VFS_MAX_PATH_LENGTH
    );
    vfs_build_absolute_path(
        vfs_get_real_root(),
        target,
        target_path,
        VFS_MAX_PATH_LENGTH
    );
    _debug_printf(
        "Attempting to mount device %s at target %s\n",
        device_path,
        target_path
    );
    kfree(device_path);
    kfree(target_path);

    // First check: ext2 must have superblock at byte 1024, with size=1024 B
    if (device_file->inode->length < 2048) {
        _debug_printf(
            "Device file is too small: %u B\n",
            device_file->inode->length
        );
        return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
    }

    vfs_file_t* file = vfs_open(device_file, VFS_MODE_READONLY);
    if (file == NULL) {
        _debug_puts("result of vfs_open is NULL");
        return MOUNT_ERR_NULL_POINTER;
    }

    // Locate the superblock
    vfs_seek(file, 1024);
    _debug_printf("size of superblock: %d\n", sizeof(ext2_superblock_t));
    uint8_t* buf = kmalloc_flags(sizeof(uint8_t) * 1024, KMALLOC_ZERO);
    vfs_read(file, 1024, buf);

    ext2_superblock_t* superblock = (ext2_superblock_t*)buf;
    if (superblock->signature != 0xEF53) {
        _debug_printf(
            "Invalid signature: 0x%04X, 0xEF53 expected\n",
            superblock->signature
        );

        vfs_close(file);
        kfree(buf);

        return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
    }

    _debug_puts("Signature valid");

    int group_count_from_blocks = (int)ceil(
        ((double)superblock->total_blocks) / superblock->blocks_per_group
    );
    int group_count_from_inodes = (int)ceil(
        ((double)superblock->total_inodes) / superblock->inodes_per_group
    );

    _debug_printf(
        "Total blocks = %u, blocks per group = %u, est. group count = %u\n",
        superblock->total_blocks,
        superblock->blocks_per_group,
        group_count_from_blocks
    );

    _debug_printf(
        "Total inodes = %u, inodes per group = %u, est. group count = %u\n",
        superblock->total_inodes,
        superblock->inodes_per_group,
        group_count_from_inodes
    );

    if (group_count_from_blocks != group_count_from_inodes) {
        _debug_puts("Group count from blocks and inodes doesn't match");
        kfree(buf);
        vfs_close(file);
        return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
    }

    _debug_printf(
        "Last written timestamp: %u\n",
        superblock->last_written_time
    );

    vfs_node_t* inode = target->inode;
    inode->inode = 2; // Root directory inode
    ext2_inode_metadata_t* metadata = kmalloc(sizeof(ext2_inode_metadata_t));
    metadata->device_file = device_file;
    // hold mount reference; released when ext2 unmounts
    device_file->inode->open_count++;
    metadata->inodes_per_group = superblock->inodes_per_group;
    metadata->block_size = 1024u << superblock->block_size;
    metadata->inode_size =
        superblock->major_version >= 1 ? superblock->inode_size : 128;
    _debug_printf(
        "mount: block_size=%u inode_size=%u major_version=%u\n",
        metadata->block_size, metadata->inode_size, superblock->major_version
    );
    inode->metadata = metadata;
    inode->on_release = ext2_on_release;
    inode->readdir_node = ext2_readdir;
    inode->finddir_node = ext2_finddir;

    kfree(buf);
    vfs_close(file);

    return MOUNT_SUCCESS;
}
