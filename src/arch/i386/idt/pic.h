#ifndef ARCH_I386_PIC_H
#define ARCH_I386_PIC_H

/**
 * Following #define group can be used for:
 * - pic_send_eoi
 * - pic_enable
 * - pic_disable
 */
// Master PIC
// Programmable Interrupt Timer
#define PIC_LINE_PIT 0
#define PIC_LINE_PS2_KEYBOARD 1
// Cascade (used by slave PIC)
#define PIC_LINE_CASCADE 2
#define PIC_LINE_COM2 3
#define PIC_LINE_COM1 4
#define PIC_LINE_LPT2 5
#define PIC_LINE_FLOPPY_DISK 6
// Also used as "spurious" interrupt
#define PIC_LINE_LPT1 7

// Slave PIC
// CMOS Real-time clock
#define PIC_LINE_CMOS 8
// Free for peripherals / legacy SCSI / NIC
#define PIC_LINE_FREE_1 9
// Free for peripherals / SCSI / NIC
#define PIC_LINE_FREE_2 10
// Free for peripherals / SCSI / NIC
#define PIC_LINE_FREE_3 11
#define PIC_LINE_PS2_MOUSE 12
// FPU / Coprocessor / Inter-processor
#define PIC_LINE_FPU 13
#define PIC_LINE_PRIMARY_ATA 14
#define PIC_LINE_SECONDARY_ATA 15


#include <stdint.h>

/**
 * Remapping PIC to selected interrupt numbers. Needs to be done once
 * IDT is initialized.
 */
void pic_remap(uint8_t, uint8_t);

/**
 * Send end-of-interrupt signal to PIC, informing it that it can continue
 * sending interrupts.
 */
void pic_send_eoi(uint8_t);

/**
 * Enable receiving interrupts on a selected PIC line.
 */
void pic_enable(uint8_t line);

/**
 * Disable receiving interrupts on a selected PIC line.
 */
void pic_disable(uint8_t line);

#endif
