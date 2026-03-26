#ifndef KERNEL_DRIVERS_PIT_H
#define KERNEL_DRIVERS_PIT_H

#include <stdint.h>

// Timeout callback function
typedef void (*timeout_callback_t)(void* data);

void pit_init();

/**
 * Register a new timeout that will be called after a given time, with selected
 * data. Returns the ID of the timeout, that can be referenced in the
 * pit_cancel_timeout function, or -1 if there are no more timeouts available.
 */
int pit_register_timeout(
    uint32_t ms,
    timeout_callback_t callback,
    void* data
);

/**
 * Cancels selected timeout by ID.
 */
void pit_cancel_timeout(int id);
#endif
