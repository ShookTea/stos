#if !(defined(__is_libk))

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>

// Track current program break
static void* current_brk = NULL;
static bool brk_initialized = false;

void* sbrk(intptr_t increment)
{
    if (!brk_initialized) {
        current_brk = brk(NULL);
        brk_initialized = true;
    }

    // If increment is zero, return current pointer
    if (increment == 0) {
        return current_brk;
    }

    // Calculate new break
    void* new_brk = (void*)((uintptr_t)current_brk + increment);
    void* result = brk(new_brk);

    if (result == current_brk && increment != 0) {
        // Failed to change (kernel returned old value)
        return (void*)-1;
    }

    void* old_brk = current_brk;
    current_brk = result;
    return old_brk;
}

#endif
