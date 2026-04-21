#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include "../../io.h"
#include "kernel/memory/kmalloc.h"
#include "./atapi/atapi.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define _debug_puts(...) debug_puts_c("ATA/id", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA/id", __VA_ARGS__)

#define ATA_CYLINDER_PIO 0x0000
#define ATA_CYLINDER_SATA 0xC33C
#define ATA_CYLINDER_ATAPI 0xEB14

static uint8_t selected_drive = ATA_DRIVE_NONE;
static ata_disk_info_t* disk_info = NULL;

static void _ata_pio_identify(uint16_t bus_base, uint8_t target_drive)
{
    _debug_puts("Detecting ATA PIO device");
    bool primary = bus_base == ATA_BUS_BASE_PRIMARY;
    bool master = target_drive == ATA_COM_TARGET_DRIVE_MASTER;
    uint8_t disk_id = (primary ? 0b00 : 0b10) | (master ? 0b00 : 0b01);

    uint8_t status = _ata_read_status(bus_base);
    // Continue polling until bit DRQ or ERR sets.
    while ((status & (ATA_STATUS_ERR | ATA_STATUS_DRQ)) == 0) {
        io_wait();
        status = _ata_read_status(bus_base);
        // TODO: introduce timeout here
    }

    if (status & ATA_STATUS_ERR) {
        _debug_puts("Invalid device: disk reports an error.");
        return;
    }

    // Data port now contains 256 16-bit values.
    uint32_t lba28_sectors_count = 0;
    uint64_t lba48_sectors_count = 0;
    char firmware_name[40];
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        // if i=0 - it's apparently useful if device is not a hard disk.
        if (i == 88) {
            // Checking UDMA modes - low byte shows supported, high byte shows
            // the active mode.
            #if KERNEL_DEBUG_ANY
                uint8_t udma_sup = data & 0xFF;
                uint8_t udma_act = (data >> 8) & 0xFF;
                _debug_printf("UDMA sup=%#02x act=%#02x\n", udma_sup, udma_act);
            #endif
        } else if (i == 60) {
            // LBA28 sectors count
            lba28_sectors_count = data;
        } else if (i == 61) {
            lba28_sectors_count |= ((uint32_t)data << 16);
            if (lba28_sectors_count == 0) {
                _debug_puts("LBA28 not supported");
            } else {
                _debug_printf("LBA28 sectors count: %u\n", lba28_sectors_count);
            }
        } else if (i == 100) {
            // LBA48 sectors count
            lba48_sectors_count = data;
        } else if (i == 101) {
            lba48_sectors_count |= (((uint64_t)data) << 16);
        } else if (i == 102) {
            lba48_sectors_count |= (((uint64_t)data) << 32);
        } else if (i == 103) {
            lba48_sectors_count |= (((uint64_t)data) << 48);
            if (lba48_sectors_count == 0) {
                _debug_puts("LBA48 not supported");
            } else {
                _debug_printf("LBA48 sec count: %llu\n", lba48_sectors_count);
            }
        } else if (i >= 27 && i <= 46) {
            firmware_name[(i - 27) * 2] = (data >> 8) & 0xFF;
            firmware_name[(i - 27) * 2 + 1] = data & 0xFF;
            if (i == 46) {
                // Set last char to NULL
                firmware_name[39] = 0;
                // Iterate from back, clearing all whitespaces
                size_t i = 38;
                while (firmware_name[i] == ' ') {
                    firmware_name[i] = 0;
                    i--;
                }
                _debug_printf("Firmware: '%s'\n", firmware_name);
            }
        }
    }

    ata_disk_info_t di;
    di.type = PIO;
    di.sectors_count = lba28_sectors_count;
    di.sector_size = 512; // It's standard for ATA disks
    strcpy(di.firmware_name, firmware_name);
    _ata_save_disk_info(disk_id, &di);

    _debug_printf("Drive found, status: %#x\n", status);
}

static void _ata_identify(uint16_t bus_base, uint8_t target_drive)
{
    _ata_drive_select(bus_base, target_drive);
    uint8_t status = _ata_read_status(bus_base);
    if (status == 0) {
        _debug_puts("Drive doesn't exist.");
        return;
    }

    // Clear values on ports
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_COUNT, 0);
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_NUMBER, 0);
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW, 0);
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH, 0);
    // Send IDENTIFY command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_IDENTIFY);

    // Read status
    status = _ata_read_status(bus_base);
    if (status == 0) {
        _debug_puts("Drive doesn't exist.");
        return;
    }

    // Wait for BSY flag to clear
    status = _ata_wait_for_bsy_clear(bus_base);
    uint16_t cylinder = (inb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH) << 8)
        | inb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW);


    if (cylinder == ATA_CYLINDER_PIO) {
        _ata_pio_identify(bus_base, target_drive);
    } else if (cylinder == ATA_CYLINDER_ATAPI) {
        _atapi_identify(bus_base, target_drive);
    } else if (cylinder == ATA_CYLINDER_SATA) {
        _debug_puts("SATA device detected but not supported yet");
    } else {
        _debug_printf("Invalid device: cylinder set to %04x\n", cylinder);
    }
}

void _ata_identify_devices()
{
    if (disk_info != NULL) {
        _debug_puts("ATA devices were identified already - skipping");
        return;
    }
    disk_info = kmalloc_flags(sizeof(ata_disk_info_t) * 4, KMALLOC_ZERO);
    _debug_puts("  Identifying master drive at primary port");
    _ata_identify(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_MASTER);
    _debug_puts("  Identifying slave drive at primary port");
    _ata_identify(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_SLAVE);
    _debug_puts("  Identifying master drive at secondary port");
    _ata_identify(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_MASTER);
    _debug_puts("  Identifying slave drive at secondary port");
    _ata_identify(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_SLAVE);

    // Selecting first available drive
    if (disk_info[ATA_DRIVE_PRIMARY_MASTER].type != NOT_PRESENT) {
        _ata_drive_select(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_MASTER);
        selected_drive = ATA_DRIVE_PRIMARY_MASTER;
        _debug_puts("Primary/master drive selected");
    } else if (disk_info[ATA_DRIVE_PRIMARY_SLAVE].type != NOT_PRESENT) {
        _ata_drive_select(ATA_BUS_BASE_PRIMARY, ATA_COM_TARGET_DRIVE_SLAVE);
        selected_drive = ATA_DRIVE_PRIMARY_SLAVE;
        _debug_puts("Primary/slave drive selected");
    } else if (disk_info[ATA_DRIVE_SECONDARY_MASTER].type != NOT_PRESENT) {
        _ata_drive_select(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_MASTER);
        selected_drive = ATA_DRIVE_SECONDARY_MASTER;
        _debug_puts("Secondary/master drive selected");
    } else if (disk_info[ATA_DRIVE_SECONDARY_SLAVE].type != NOT_PRESENT) {
        _ata_drive_select(ATA_BUS_BASE_SECONDARY, ATA_COM_TARGET_DRIVE_SLAVE);
        selected_drive = ATA_DRIVE_SECONDARY_SLAVE;
        _debug_puts("Secondary/slave drive selected");
    } else {
        _debug_puts("No driver selected");
    }
}

uint8_t ata_get_selected_drive(void)
{
    return selected_drive;
}

void ata_get_available_drives(uint8_t* res)
{
    memset(res, ATA_DRIVE_NONE, sizeof(uint8_t) * 5);
    size_t idx = 0;
    if (disk_info == NULL) {
        return;
    }
    if (disk_info[ATA_DRIVE_PRIMARY_MASTER].type != NOT_PRESENT) {
        res[idx++] = ATA_DRIVE_PRIMARY_MASTER;
    }
    if (disk_info[ATA_DRIVE_PRIMARY_SLAVE].type != NOT_PRESENT) {
        res[idx++] = ATA_DRIVE_PRIMARY_SLAVE;
    }
    if (disk_info[ATA_DRIVE_SECONDARY_MASTER].type != NOT_PRESENT) {
        res[idx++] = ATA_DRIVE_SECONDARY_MASTER;
    }
    if (disk_info[ATA_DRIVE_SECONDARY_SLAVE].type != NOT_PRESENT) {
        res[idx++] = ATA_DRIVE_SECONDARY_SLAVE;
    }
}

void ata_select_drive(uint8_t drive)
{
    selected_drive = drive;
    bool primary = ata_drive_is_primary(drive);
    bool master = ata_drive_is_master(drive);
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;
    _ata_drive_select(
        bus_base,
        master ? ATA_COM_TARGET_DRIVE_MASTER : ATA_COM_TARGET_DRIVE_SLAVE
    );
}

uint8_t ata_parse_mbr_partitions(
    const uint8_t* mbr_buffer,
    ata_partition_t* out_parts
) {
    const ata_mbr_t* mbr = (const ata_mbr_t*)mbr_buffer;
    if (mbr->signature_bytes != 0xAA55) {
        return 0;
    }
    const ata_mbr_partition_table_entry_t* entries[4] = {
        &mbr->partition_1, &mbr->partition_2,
        &mbr->partition_3, &mbr->partition_4,
    };
    uint8_t count = 0;
    for (int i = 0; i < 4; i++) {
        const ata_mbr_partition_table_entry_t* pte = entries[i];
        if (pte->partition_type == 0) continue;
        out_parts[count].type = pte->partition_type;
        out_parts[count].lba_start = pte->partition_start_lba;
        out_parts[count].sectors_count = pte->partition_sectors_count;
        out_parts[count].bootable = pte->drive_attributes & 0x80;
        count++;
    }
    return count;
}

void _ata_save_disk_info(const uint8_t drive_id, const ata_disk_info_t* src)
{
    if (drive_id >= ATA_DRIVE_NONE) {
        return;
    }
    memcpy(&disk_info[drive_id], src, sizeof(ata_disk_info_t));
}

bool ata_load_disk_info(uint8_t disk_id, ata_disk_info_t* ptr)
{
    if (disk_id >= ATA_DRIVE_NONE) {
        return false;
    }
    if (disk_info[disk_id].type == NOT_PRESENT) {
        return false;
    }

    memcpy(ptr, &disk_info[disk_id], sizeof(ata_disk_info_t));

    return true;
}
