#ifndef KERNEL_TASK_USERMODE_H
#define KERNEL_TASK_USERMODE_H

/**
 * Enters usermode, with given entrypoint and stack.
 */
void usermode_enter(void* entry, void* stack);

#endif
