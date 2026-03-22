#include "task.h"
#include <kernel/memory/kmalloc.h>
#include <string.h>

/**
 * Build initial stack frame (as if the task was interrupted).
 */
static void task_setup_initial_stack(
    task_t* task,
    void (*entrypoint)(),
    bool is_kernel
) {
    // Get top of the kernel stack. Stack grows downward, so we push in reverse
    // order.
    uint32_t* stack = (uint32_t*)(
        task->kernel_stack_base + task->kernel_stack_size
    );

    if (!is_kernel) {
        // User mode task - need SS:ESP for IRET
        stack--;
        *stack = 0x23; // User data segment (GDT entry 4, RPL=3)
        stack--;
        *stack = task->user_stack_base + task->user_stack_size; // User ESP
    }

    // IRET frame (always present)
    stack--;
    *stack = 0x202; // EFLAGS (IF=1, reserved bit=1)

    stack--;
    if (is_kernel) {
        *stack = 0x08; // Kernel code segment
    } else {
        *stack = 0x1B; // User code segment (GDT entry 3, RPL=3)
    }

    stack--;
    *stack = (uint32_t)entrypoint; // EIP

    // Registers saved by PUSHA (in reverse order)
    stack--; *stack = 0; // EDI
    stack--; *stack = 0; // ESI
    stack--; *stack = 0; // EBP
    stack--; *stack = 0; // ESP placeholder, ignored by POPA
    stack--; *stack = 0; // EBX
    stack--; *stack = 0; // EDX
    stack--; *stack = 0; // ECX
    stack--; *stack = 0; // EAX

    // Segment registers, saved by interrupt handler
    if (is_kernel) {
        stack--; *stack = 0x10; // DS (kernel data)
        stack--; *stack = 0x10; // ES
        stack--; *stack = 0x10; // FS
        stack--; *stack = 0x10; // GS
    } else {
        stack--; *stack = 0x23; // DS (user data, RPL=3)
        stack--; *stack = 0x23; // ES
        stack--; *stack = 0x23; // FS
        stack--; *stack = 0x23; // GS
    }

    // Callee-saved registers (for switch_to_stack)
    stack--; *stack = 0; // EDI
    stack--; *stack = 0; // ESI
    stack--; *stack = 0; // EBX
    stack--; *stack = 0; // EBP

    // Save final ESP to task context
    task->context.esp = (uint32_t)stack;
}

task_t* task_create(const char name[64], void (*entrypoint)(), bool is_kernel)
{
    task_t* task = kmalloc_flags(sizeof(task_t), KMALLOC_ZERO);
    strcpy(task->name, name);
    task_setup_initial_stack(task, entrypoint, is_kernel);
    return task;
}

void task_destroy(task_t* task)
{
    kfree(task);
}
