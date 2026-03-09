#ifndef ARCH_I386_PIC_H
#define ARCH_I386_PIC_H

#include <stdint.h>

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

void pic_remap(uint8_t, uint8_t);

void pic_send_eoi(uint8_t);

#endif
