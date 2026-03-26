#include "pic.h"
#include <stdint.h>
#include "../io.h"

// IO base address for master PIC
#define PIC1 0x20
// IO base address for slave PIC
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)

#define PIC_EOI 0x20

// Indicates that ICW4 will be present
#define ICW1_ICW4	0x01
// Single (cascade) mode
#define ICW1_SINGLE	0x02
// Call address interval 4 (8)
#define ICW1_INTERVAL4	0x04
// Level triggered (edge) mode
#define ICW1_LEVEL	0x08
// Initialization required
#define ICW1_INIT	0x10

// 8086/88 (MCS-80/85) mode
#define ICW4_8086	0x01
// Auto (normal) EOI
#define ICW4_AUTO	0x02
// Buffered mode/slave
#define ICW4_BUF_SLAVE	0x08
// Buffered mode/master
#define ICW4_BUF_MASTER	0x0C
// Special fully nested (not)
#define ICW4_SFNM	0x10

#define CASCADE_IRQ 2

/**
 * arguments:
 * - offset1: vector offset for master PIC
 * - offset2: vector offset for slave PIC
 */
void pic_remap(uint8_t offset1, uint8_t offset2)
{
    // starts the initialization sequence (in cascade mode)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: Master PIC vector offset
    outb(PIC1_DATA, offset1);
    io_wait();
    // ICW2: Slave PIC vector offset
    outb(PIC2_DATA, offset2);
    io_wait();

    // ICW3: tell Master PIC that there is a slave PIC at IRQ2
    outb(PIC1_DATA, 1 << CASCADE_IRQ);
    io_wait();
    // ICW3: tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 2);
    io_wait();

    // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Mask both PICs, so they're off by default
    outb(PIC1_DATA, 0xFB);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_enable(uint8_t line)
{
    uint16_t port;
    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    uint8_t value = inb(port);
    outb(port, value & ~(1 << line));
}

void pic_disable(uint8_t line)
{
    uint16_t port;
    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }

    uint8_t value = inb(port);
    outb(port, value | (1 << line));
}
