#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

#include <stdint.h>

typedef struct {
    volatile uint32_t lock; // 0 = unlocked, 1 = locked
    volatile uint32_t flags; // saved EFLAGS
} spinlock_t;

#define SPINLOCK_INIT { .lock = 0, .flags = 0 }

/**
 * Acquires the spinlock. Will halt the process if the spinlock is taken,
 * waiting for it to be released
 */
void spinlock_acquire(spinlock_t* lock);

/**
 * Releases the spinlock
 */
void spinlock_release(spinlock_t* lock);

#endif
