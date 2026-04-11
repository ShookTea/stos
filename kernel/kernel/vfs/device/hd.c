#include "../device.h"
#include "kernel/debug.h"
#include "kernel/drivers/ata.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

vfs_node_t** device_hd_mount()
{
    _debug_puts("Mounting HD files to VFS");
    uint8_t ata_drives[5];
    ata_get_available_drives(ata_drives);
    uint8_t* ata_drive_ptr = ata_drives;
    char drive_letter = 'a';
    while (*ata_drive_ptr != 0) {
        _debug_printf(
            "drive with type %d found, mounting to /dev/hd%c\n",
            *ata_drive_ptr,
            drive_letter
        );
        drive_letter++;
        ata_drive_ptr++;
    }
    return NULL;
}

void device_hd_unmount()
{}
