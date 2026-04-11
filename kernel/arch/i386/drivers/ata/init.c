#include "kernel/drivers/ata.h"
#include "kernel/debug.h"
#include "./common.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../idt/pic.h"
#include "../../idt/idt.h"
#include "kernel/memory/kmalloc.h"
#include <libds/libds.h>
#include <libds/ringbuf.h>

#define _debug_puts(...) debug_puts_c("ATA", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ATA", __VA_ARGS__)

static uint16_t* buffer = NULL;

void _test_read_callback(void* data __attribute__((unused)))
{
    _debug_puts("Read completed, data:");
    for (size_t i = 0; i < 512; i++) {
        if (i % 16 == 0) {
            debug_puts("");
        }
        if (i == 256) {
            debug_puts("");
        }
        debug_printf("%04x ", buffer[i]);
    }
    debug_puts("");
    kfree(buffer);
}

void _test_write_callback(void* data __attribute__((unused)))
{
    _debug_puts("Writing completed.");
    kfree(buffer);
}

void _test_read()
{
    buffer = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    ata_read(15, 2, buffer, _test_read_callback, NULL);
}

void _test_write()
{
    buffer = kmalloc_flags(sizeof(uint16_t) * 256 * 2, KMALLOC_ZERO);
    buffer[0] = 0xCAFE;
    buffer[1] = 0xBABE;
    buffer[256] = 0xDEAD;
    buffer[257] = 0xBEEF;
    buffer[4] = 0xCAFE;
    buffer[258] = 0xCAFE;
    ata_write(15, 2, buffer, _test_write_callback, NULL);
}

void ata_init()
{
    _debug_puts("Initializing ATA");
    _ata_identify_devices();
    uint8_t drive = ata_get_selected_drive();
    if (drive == ATA_DRIVE_NONE) {
        return;
    }
    if (drive == ATA_DRIVE_PRIMARY_MASTER || drive == ATA_DRIVE_PRIMARY_SLAVE) {
        idt_register_irq_handler(PIC_LINE_PRIMARY_ATA, &_ata_irq_handler);
        pic_enable(PIC_LINE_PRIMARY_ATA);
    } else {
        idt_register_irq_handler(PIC_LINE_SECONDARY_ATA, &_ata_irq_handler);
        pic_enable(PIC_LINE_SECONDARY_ATA);
    }
    _debug_puts("IRQ enabled");

    // _test_read();
    // _test_write();
}
