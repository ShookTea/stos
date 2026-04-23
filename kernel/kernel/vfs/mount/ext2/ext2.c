#include "ext2.h"
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/debug.h"
#include <stdint.h>
#include <math.h>

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2", __VA_ARGS__)

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

    _debug_printf(
        "Total blocks = %u, blocks per group = %u, est. group count = %u\n",
        superblock->total_blocks,
        superblock->blocks_per_group,
        (int)ceil(
            ((double)superblock->total_blocks) / superblock->blocks_per_group
        )
    );

    _debug_printf(
        "Total inodes = %u, inodes per group = %u, est. group count = %u\n",
        superblock->total_inodes,
        superblock->inodes_per_group,
        (int)ceil(
            ((double)superblock->total_inodes) / superblock->inodes_per_group
        )
    );

    vfs_close(file);
    kfree(buf);

    return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
}
