#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_ISO9660_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_ISO9660_H

#include "kernel/vfs/vfs.h"
#include "kernel/task/wait.h"
#include <stdint.h>

#define ISO_SECTOR_SIZE 2048

/**
 * Metadata used for nodes in ISO-9660 format
 */
typedef struct {
    // ISO device file
    dentry_t* device_file;
    uint32_t extent_lba;
    uint32_t extent_size;
} iso9660_node_t;

// Structure for the mounting queue
typedef struct {
    dentry_t* device_file;
    dentry_t* target;
    uint8_t* buffer;
    wait_obj_t* wait_obj;
    bool completed;
    vfs_mount_result_t result;
} mount_task_t;

#define ISO_DIR_FLAG_HIDDEN 0x01
#define ISO_DIR_FLAG_SUBDIRECTORY 0x02
#define ISO_DIR_FLAG_ASSOCIATED_FILE 0x04
// Extended Attribute Record contains information about the format of the file
#define ISO_DIR_FLAG_EAR_FORMAT 0x08
// EAR contains owner and group permissions
#define ISO_DIR_FLAG_EAR_PERMISSIONS 0x10
#define ISO_DIR_FLAG_FINAL_DIRECTORY 0x80

typedef struct __attribute__((packed)) {
    uint8_t directory_record_length;
    uint8_t extended_attribute_record_length;
    uint32_t extent_lba;
    uint32_t __unused1; // extent_lba in big-endian
    uint32_t extent_size;
    uint32_t __unused2; // extent_size in big-endian
    uint8_t recording_datetime[7];
    uint8_t file_flags; // ISO_DIR_FLAG_ values
    uint8_t file_unit_size; // only for files in interleaved mode
    uint8_t interleave_gap_size;
    uint16_t volume_seq_number;
    uint16_t __unused3; // volume_seq_number in big-endian
    uint8_t file_name_length;
    char file_name[0];
} iso_dir_record_t;

#endif
