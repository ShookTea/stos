#ifndef ARCH_I386_PIC_H
#define ARCH_I386_PIC_H

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

#endif
