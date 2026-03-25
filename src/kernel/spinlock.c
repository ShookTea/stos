#include <kernel/spinlock.h>

void spinlock_acquire(spinlock_t* lock) {
    // Save interrupt flags
    uint32_t flags;
    __asm__ volatile(
        "pushfl\n"
        "popl %0\n"
        "cli\n"
        : "=r"(flags)
        :
        : "memory"
    );

    // Try to acquire the lock
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        // The lock is held by someone else, keep spinning
        __asm__ volatile("pause");
    }

    // Store flags for restoration later
    lock->flags = flags;
}

void spinlock_release(spinlock_t *lock)
{
    // Save interrupt flags needed for restoration
    uint32_t flags = lock->flags;

    // Release the clock (atomic operation)
    __sync_lock_release(&lock->lock);

    // Restore interrupt flags
    __asm__ volatile(
        "pushl %0\n"
        "popfl\n"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}
