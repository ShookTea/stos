#ifndef KERNEL_MEMORY_TSS_H
#define KERNEL_MEMORY_TSS_H

#include <stdint.h>

/**
 * Set kernal stack in TSS. This needs to be called during task swtich to
 * update ESP0.
 */
void tss_set_kernel_stack(
    uint32_t kernel_stack_base,
    uint32_t kernel_stack_size
);

#endif
