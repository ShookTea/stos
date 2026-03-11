#include <kernel/drivers/ps2.h>
#include <stdbool.h>
#include "ps2_defines.h"
#include "../io.h"
#include <stdio.h>

static bool initialized = false;
static bool dual_channel = false;
static bool port1_online = false;
static bool port2_online = false;

static void ps2_send_com(uint8_t command)
{
    // Before sending a command, wait for input buffer to be empty
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER) {
        io_wait();
    }
    outb(PS2_COMMAND_PORT, command);
}

static void ps2_send_data(uint8_t command)
{
    // Before sending a command, wait for input buffer to be empty
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER) {
        printf(".");
        io_wait();
    }
    outb(PS2_DATA_PORT, command);
}

static uint8_t ps2_read_data()
{
    // Wait for anything to appear in the output buffer
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER)) {
        io_wait();
    }
    return inb(PS2_DATA_PORT);
}

void ps2_init()
{
    if (initialized) {
        return;
    }
    initialized = true;

    // First, disable devices (so they won't send data at the wrong time)
    ps2_send_com(PS2_COM_DISABLE_PORT_1);
    ps2_send_com(PS2_COM_DISABLE_PORT_2);

    // Flush the output buffer - read from data port for as long as there's
    // anything on output
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER) {
        inb(PS2_DATA_PORT);
    }

    // Set the controller configuration byte: read current value, and then clear
    // bits 0 (disabling port interrupt), 6 (disabling port translation), and
    // 4 (enabling port clock)
    ps2_send_com(PS2_COM_READ_CCB);
    uint8_t ccb = ps2_read_data();
    ccb &= ~0b01010001;
    ps2_send_com(PS2_COM_SET_CCB);
    ps2_send_data(ccb);

    // Run self-test of the PS2 controller
    ps2_send_com(PS2_COM_TEST_CONTROLLER);
    if (ps2_read_data() != 0x55) {
        // TODO: what should we really do now?
        puts("Warning! PS/2 controller self-test failed");
        return;
    } else {
        puts("PS/2 controller self-test completed");
    }

    // Checking if PS/2 is a dual channel: we try to enable port 2, then we
    // read the CCB again. If bit 5 is set, that means that it's not a dual
    // channel.
    // Otherwise, we should again disable the port and clear bits 1 and 5
    // of the CCB to disable IRQ and enable clock for port 2. No need to
    // disabling translations, because second port never supports it.
    ps2_send_com(PS2_COM_ENABLE_PORT_2);
    ps2_send_com(PS2_COM_READ_CCB);
    ccb = ps2_read_data();
    if (ccb & 0b00010000) {
        puts("Single-channel PS/2 controller detected");
    } else {
        puts("Dual-channel PS/2 controller detected");
        dual_channel = true;
        ps2_send_com(PS2_COM_DISABLE_PORT_2);
        ccb &= ~0b00100010;
        ps2_send_com(PS2_COM_SET_CCB);
        ps2_send_data(ccb);
    }

    // Run self-test of port 1
    ps2_send_com(PS2_COM_TEST_PORT_1);
    if (ps2_read_data() == 0x00) {
        puts("PS/2 port 1 test completed");
        port1_online = true;
    } else {
        puts("Warning! PS/2 port 1 test failed");
    }

    // If port 2 is present, run test for port 2 as well
    ps2_send_com(PS2_COM_TEST_PORT_2);
    if (ps2_read_data() == 0x00) {
        puts("PS/2 port 2 test completed");
        port2_online = true;
    } else {
        puts("Warning! PS/2 port 2 test failed");
    }
}
