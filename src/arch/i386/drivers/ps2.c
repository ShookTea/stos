#include <kernel/drivers/ps2.h>
#include <stdbool.h>
#include "ps2_defines.h"
#include "../io.h"
#include <stdio.h>

static bool initialized = false;

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
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER) {
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

    puts("setting config byte");

    // Set the controller configuration byte: read current value, and then clear
    // bits 0 (disabling port interrupt) and 6 (disabling port translation), and
    // setting bit 4 (enabling port clock)
    ps2_send_com(PS2_COM_READ_CCB);
    uint8_t ccb = ps2_read_data();
    ccb = (ccb & ~0x41) | 0x10;
    ps2_send_com(PS2_COM_SET_CCB);
    ps2_send_data(ccb);

    puts("Config byte set");
}
