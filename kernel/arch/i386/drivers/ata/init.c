#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../idt/pic.h"
#include "../../idt/idt.h"
#include "../../io.h"
#include "kernel/memory/kmalloc.h"

static void _irq_handler()
{
    uint8_t drive = ata_get_selected_drive();
    bool primary = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_PRIMARY_SLAVE;
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    debug_puts("Received IRQ event on ATA");
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(bus_base | ATA_BUS_OFFSET_DATA);
        debug_printf("%04x ", data);
        if (i % 16 == 15) {
            debug_puts("");
        }
    }
    // Read status register to acknowledge
    _ata_read_status(bus_base);
    debug_puts("Reading completed.");
}

/**
 * Start read command on given sector
 * lba - first sector ID
 * sector_count - number of sectors to read
 */
void _test_read(uint32_t lba, uint8_t sector_count)
{
    // TODO: checking ATA status when reading needed
    uint8_t drive = ata_get_selected_drive();
    bool primary = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_PRIMARY_SLAVE;
    bool master = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_SECONDARY_MASTER;
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Select drive + top 4 bits of LBA
    outb(
        bus_base | ATA_BUS_OFFSET_DRIVE_HEAD,
        0xE0 | ((master ? 0 : 1) << 4) | ((lba >> 24) & 0x0F)
    );
    // Send sector count + LBA bits 0-23
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_COUNT,  sector_count);
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_NUMBER, (uint8_t)(lba));
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW,  (uint8_t)(lba >> 8));
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH, (uint8_t)(lba >> 16));
    // Send READ SECTORS command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_READ_SECTORS);

    // Now we'll one IRQ for each sector_count. Each such interrupt will give us
    // 256 16-bit values on port bus_base | ATA_BUS_OFFSET_DATA.
}

/**
 * Write data from "data" pointer to given address.
 */
void _test_write(uint32_t lba, uint8_t sector_count, uint16_t* data)
{
    // TODO: checking ATA status when reading needed
    uint8_t drive = ata_get_selected_drive();
    bool primary = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_PRIMARY_SLAVE;
    bool master = drive == ATA_DRIVE_PRIMARY_MASTER
        || drive == ATA_DRIVE_SECONDARY_MASTER;
    uint16_t bus_base = primary ? ATA_BUS_BASE_PRIMARY : ATA_BUS_BASE_SECONDARY;

    // Select drive + top 4 bits of LBA
    outb(
        bus_base | ATA_BUS_OFFSET_DRIVE_HEAD,
        0xE0 | ((master ? 0 : 1) << 4) | ((lba >> 24) & 0x0F)
    );
    // Send sector count + LBA bits 0-23
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_COUNT,  sector_count);
    outb(bus_base | ATA_BUS_OFFSET_SECTOR_NUMBER, (uint8_t)(lba));
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_LOW,  (uint8_t)(lba >> 8));
    outb(bus_base | ATA_BUS_OFFSET_CYLINDER_HIGH, (uint8_t)(lba >> 16));
    // Send WRITE SECTORS command
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_WRITE_SECTORS);

    // Wait for drive to be ready to accept data (DRQ set, BSY clear)
    uint8_t status;
    do {
        status = _ata_read_status(bus_base);
    } while ((status & ATA_STATUS_BSY) || !(status & ATA_STATUS_DRQ));

    // Now we need to send data to port, sector by sector
    for (int s = 0; s < sector_count; s++) {
        for (int i = 0; i < 256; i++) {
            outw(bus_base | ATA_BUS_OFFSET_DATA, data[s * 256 + i]);
            io_wait();
        }
        // Acknowledge status
        _ata_read_status(bus_base);
    }


    // Now we'll one IRQ for each sector_count. Each such interrupt will give us
    // 256 16-bit values on port bus_base | ATA_BUS_OFFSET_DATA.

    // Send FLUSH command at the end of every write
    outb(bus_base | ATA_BUS_OFFSET_COMMAND, ATA_COM_FLUSH);
}

void ata_init()
{
    debug_puts("Initializing ATA");
    _ata_identify_devices();
    uint8_t drive = ata_get_selected_drive();
    if (drive == ATA_DRIVE_NONE) {
        return;
    }
    if (drive == ATA_DRIVE_PRIMARY_MASTER || drive == ATA_DRIVE_PRIMARY_SLAVE) {
        idt_register_irq_handler(PIC_LINE_PRIMARY_ATA, &_irq_handler);
        pic_enable(PIC_LINE_PRIMARY_ATA);
    } else {
        idt_register_irq_handler(PIC_LINE_SECONDARY_ATA, &_irq_handler);
        pic_enable(PIC_LINE_SECONDARY_ATA);
    }
    debug_puts("ATA IRQ enabled");

    for (int i = 0; i < 500000; i++) {
        volatile int j __attribute__((unused)) = i * i;
        io_wait();
    }

    _test_read(15, 2);

    // uint16_t* data = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    // data[256] = 0xDEAD;
    // data[257] = 0xBEEF;
    // data[0] = 0xCAFE;
    // data[1] = 0xBABE;
    // _test_write(15, 2, data);
    // kfree(data);
}
