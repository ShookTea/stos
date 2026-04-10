#include <stdint.h>
#include <stdbool.h>
#include "../../io.h"
#include "kernel/debug.h"
#include "kernel/drivers/pit.h"
#include "stdlib.h"
#include "./common.h"

/**
 * Because of ATA quirks, there can be a long delay for the process of selecting
 * a drive, and they technically suggest that if we want to read a status, we
 * should fake a delay by reading it 15 TIMES and only paying attention to the
 * result of the last one.
 *
 * Instead we're going to set "selected_drive_X" after _ata_drive_select. If
 * we see that selected_ is different than saved_, then in _ata_read_status
 * we will call status check 15 times and update saved_. Otherwise we can just
 * run it once.
 */
static uint8_t selected_drive_primary = 0;
static uint8_t selected_drive_secondary = 0;
static uint8_t saved_drive_primary = 0;
static uint8_t saved_drive_secondary = 0;

// Those falgs need to be set to volatile,
// otherwise compiler will assume that they can't change and optimize
// timeout loops out.
static volatile bool bsy_primary_hang_timeout = false;
static volatile bool bsy_secondary_hang_timeout = false;

static void ata_handle_timeout(void* data)
{
    char* mode = data;
    if (mode[0] == 'b' && mode[1] == 'p') {
        bsy_primary_hang_timeout = true;
    } else if (mode[0] == 'b' && mode[1] == 's') {
        bsy_secondary_hang_timeout = true;
    } else {
        debug_printf("Invalid ATA timeout code: >%s<\n", data);
        abort();
    }
}

static int ata_register_timeout(char* code)
{
    return pit_register_timeout(500, ata_handle_timeout, code);
}

void _ata_drive_select(uint16_t bus_base, uint8_t drive_select)
{
    if (bus_base == ATA_BUS_BASE_PRIMARY) {
        selected_drive_primary = drive_select;
    } else {
        selected_drive_secondary = drive_select;
    }
    outb(
        bus_base | ATA_BUS_OFFSET_DRIVE_HEAD,
        drive_select
    );
}

uint8_t _ata_read_status(uint16_t bus_base)
{
    bool primary = bus_base == ATA_BUS_BASE_PRIMARY;
    bool no_change = primary
        ? (selected_drive_primary == saved_drive_primary)
        : (selected_drive_secondary == saved_drive_secondary);

    if (no_change) {
        return inb(bus_base | ATA_BUS_OFFSET_STATUS);
    }

    uint8_t to_save = primary
        ? selected_drive_primary
        : selected_drive_secondary;

    uint8_t res = 0;
    for (int i = 1; i <= 15; i++) {
        res = inb(bus_base | ATA_BUS_OFFSET_STATUS);
    }

    if (bus_base == ATA_BUS_BASE_PRIMARY) {
        saved_drive_primary = to_save;
    } else {
        saved_drive_secondary = to_save;
    }
    return res;
}

uint8_t _ata_wait_for_bsy_clear(uint16_t bus_base)
{
    bool primary = bus_base == ATA_BUS_BASE_PRIMARY;
    if (primary) {
        bsy_primary_hang_timeout = false;
    } else {
        bsy_secondary_hang_timeout = false;
    }

    uint8_t status = _ata_read_status(bus_base);
    int timeout_id = ata_register_timeout(primary ? "bp" : "bs");
    while ((status & ATA_STATUS_BSY)
        && !(primary ? bsy_primary_hang_timeout : bsy_secondary_hang_timeout)
    ) {
        io_wait();
        status = _ata_read_status(bus_base);
    }
    if (primary ? bsy_primary_hang_timeout : bsy_secondary_hang_timeout) {
        // TODO: software reset is required
        debug_puts(primary ? "ATA BSY hang primary" : "ATA BSY hang secondary");
        abort();
    }

    pit_cancel_timeout(timeout_id);
    return status;
}
