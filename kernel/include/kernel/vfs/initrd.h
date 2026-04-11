#ifndef INCLUDE_KERNEL_VFS_INITRD_H
#define INCLUDE_KERNEL_VFS_INITRD_H

#include <kernel/vfs/vfs.h>

/**
 * Attempts to load initrd (passed as .tar file) from memory and returns a valid
 * dentry_t, or NULL if initrd was not present. Will cause kernel panic if
 * initrd is present, but invalid.
 */
dentry_t* initrd_mount();

/**
 * Removes initrd from VFS and frees memory allocated for its nodes
 */
void initrd_unmount();

typedef struct {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12]; // It's in octal, because why not
    char last_mod_time[12]; // unix timestamp, but again in octal
    char checksum[8];
    char typeflag[1];
} tar_header_t;
#endif
