#include <kernel/drivers/ps2.h>
#include <kernel/drivers/pit.h>
#include <stdbool.h>
#include "ps2_defines.h"
#include "../io.h"
#include "stdlib.h"
#include <stdio.h>

static bool initialized = false;
static bool dual_channel = false;
static bool port1_online = false;
static bool port2_online = false;

// Those falgs need to be set to volatile,
// otherwise compiler will assume that they can't change and optimize
// timeout loops out.
static volatile bool send_com_timeout = false;
static volatile bool send_data_timeout = false;
static volatile bool read_data_timeout = false;
static volatile bool send_byte_port1_timeout = false;
static volatile bool send_byte_port2_timeout = false;
static volatile bool read_byte_timeout = false;

static void ps2_handle_timeout(void *data)
{
    char* mode = data;
    if (mode[0] == 'C') {
        send_com_timeout = true;
    } else if (mode[0] == 'W') {
        send_data_timeout = true;
    } else if (mode[0] == 'R') {
        read_data_timeout = true;
    } else if (mode[0] == '1') {
        send_byte_port1_timeout = true;
    } else if (mode[0] == '2') {
        send_byte_port2_timeout = true;
    } else if (mode[0] == 'B') {
        read_byte_timeout = true;
    } else {
        printf("Invalid timeout: %s\n", mode);
        abort();
    }
}

static int ps2_register_timeout(char* code)
{
    return pit_register_timeout(500, ps2_handle_timeout, code);
}

static void ps2_send_com(uint8_t command)
{
    // Before sending a command, wait for input buffer to be empty
    send_com_timeout = false;
    int timeout_id = ps2_register_timeout("C");
    while (
        (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER)
        && !send_com_timeout) {
        io_wait();
    }
    if (send_com_timeout) {
        // TODO: some more safe handling
        puts("!!! ps2_send_com timeout");
        abort();
    } else {
        pit_cancel_timeout(timeout_id);
        outb(PS2_COMMAND_PORT, command);
    }
}

static void ps2_send_data(uint8_t command)
{
    // Before sending a command, wait for input buffer to be empty
    send_data_timeout = false;
    int timeout_id = ps2_register_timeout("W");
    while (
        (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER)
        && !send_data_timeout
    ) {
        io_wait();
    }
    if (send_data_timeout) {
        // TODO: some more safe handling
        puts("!!! ps2_send_data timeout");
        abort();
    } else {
        pit_cancel_timeout(timeout_id);
        outb(PS2_DATA_PORT, command);
    }
}

static uint8_t ps2_read_data()
{
    // Wait for anything to appear in the output buffer
    read_data_timeout = false;
    int timeout_id = ps2_register_timeout("R");
    while (
        !(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER)
        && !read_data_timeout
    ) {
        io_wait();
    }
    if (read_data_timeout) {
        // TODO: some more safe handling
        puts("!!! ps2_read_data timeout");
        abort();
    } else {
        pit_cancel_timeout(timeout_id);
        return inb(PS2_DATA_PORT);
    }
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

    if (!port1_online && !port2_online) {
        // TODO: what should we really do now?
        puts("Warning! No PS/2 port available.");
        return;
    }

    // Enable devices
    ps2_send_com(PS2_COM_READ_CCB);
    ccb = ps2_read_data();
    if (port1_online) {
        ps2_send_com(PS2_COM_ENABLE_PORT_1);
        ccb |= 0b00000001;
    }
    if (port2_online) {
        ps2_send_com(PS2_COM_ENABLE_PORT_2);
        ccb |= 0b00000001;
    }
    ps2_send_com(PS2_COM_SET_CCB);
    ps2_send_data(ccb);

    puts("PS/2 port(s) enabled");

    uint8_t result = 0;
    ps2_send_byte_port1(0xF5);
    if (ps2_read_byte_to_result(&result) != PS2_RESPONSE_OK) {
        puts("fail!");
    }
    printf("%#02x\n", result);
    ps2_send_byte_port1(0xEE);
    if (ps2_read_byte_to_result(&result) != PS2_RESPONSE_OK) {
        puts("fail!");
    }
    printf("%#02x\n", result);
    if (ps2_read_byte_to_result(&result) != PS2_RESPONSE_OK) {
        puts("fail!");
    }
    printf("%#02x\n", result);
    if (ps2_read_byte_to_result(&result) != PS2_RESPONSE_OK) {
        puts("fail!");
    }
    printf("%#02x\n", result);
}

/**
 * TODO:
 * So far it looks exactly the same as ps2_send_data, except it returns status
 * code on timeout instead of aborting. Maybe it should be cleaned up?
 */
uint8_t ps2_send_byte_port1(uint8_t byte)
{
    if (!port1_online) {
        return PS2_RESPONSE_UNAVAILABLE;
    }
    send_byte_port1_timeout = false;
    int timeout_id = ps2_register_timeout("1");
    while (
        (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER)
        && !send_byte_port1_timeout
    ) {
        io_wait();
    }
    if (send_byte_port1_timeout) {
        return PS2_RESPONSE_TIMEOUT;
    } else {
        pit_cancel_timeout(timeout_id);
        outb(PS2_DATA_PORT, byte);
        return PS2_RESPONSE_OK;
    }
}

/**
 * This one is more complex than sending to the first port: we first need
 * to send a command to controller saying that the byte should be sent to
 * the second port.
 */
uint8_t ps2_send_byte_port2(uint8_t byte)
{
    if (!port2_online) {
        return PS2_RESPONSE_UNAVAILABLE;
    }
    ps2_send_com(PS2_COM_SEND_BYTE_TO_PORT_2);
    send_byte_port2_timeout = false;
    int timeout_id = ps2_register_timeout("2");
    while (
        (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER)
        && !send_byte_port2_timeout
    ) {
        io_wait();
    }
    if (send_byte_port2_timeout) {
        return PS2_RESPONSE_TIMEOUT;
    } else {
        pit_cancel_timeout(timeout_id);
        outb(PS2_DATA_PORT, byte);
        return PS2_RESPONSE_OK;
    }
}

/**
 * TODO:
 * So far it looks exactly the same as ps2_read_data, except for how it handles
 * returns. Maybe it can be simplified?
 */
uint8_t ps2_read_byte_to_result(uint8_t* result)
{
    // Wait for anything to appear in the output buffer
    read_byte_timeout = false;
    int timeout_id = ps2_register_timeout("B");
    while (
        !(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER)
        && !read_byte_timeout
    ) {
        io_wait();
    }
    if (read_byte_timeout) {
        return PS2_RESPONSE_TIMEOUT;
    } else {
        pit_cancel_timeout(timeout_id);
        *result = inb(PS2_DATA_PORT);
        return PS2_RESPONSE_OK;
    }
}
