#include "task.h"
#include <kernel/memory/vmm.h>
#include <stdlib.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/paging.h>
#include <string.h>

/** When increasing tasks list length, use this size */
#define TASKS_LIST_REALLOC_SIZE 10

static task_t** tasks = NULL;
// Length of tasks list
static uint32_t tasks_length = 0;
// The number of tasks that weren't destroyed yet
static uint32_t tasks_present = 0;

#define KERNEL_STACK_SIZE (16 * 1024) // 16 KiB

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

    stack--; *stack = (uint32_t)entrypoint; // EIP
    stack--; *stack = 0; // err_code
    stack--; *stack = 0; // int_no

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

/**
 * Allocate new task in the tasks array, with given name, and set PID. Grow
 * tasks array if necessary; otherwise reuse the already existing entries.
 */
static task_t* task_allocate(const char name[64])
{
    // Allocate task data in memory
    task_t* task = kmalloc_flags(sizeof(task_t), KMALLOC_ZERO);
    strcpy(task->name, name);

    if (tasks_length == tasks_present) {
        // Need to increase the size of tasks array by some constant margin
        uint32_t new_size = tasks_length + TASKS_LIST_REALLOC_SIZE;
        tasks = krealloc(tasks, sizeof(task_t*) * new_size);

        // Clear newly allocated memory to zero it out, which can be used later
        // to check if entry is empty (NULL) or not
        memset(tasks + tasks_length, 0, TASKS_LIST_REALLOC_SIZE);
        tasks_length = new_size;

        // tasks_present is now guaranteed to be equal to first empty entry
        // and can be used as PID
        tasks[tasks_present] = task;
        task->pid = tasks_present;
        tasks_present++;
    } else {
        // There is some entry in tasks array that is empty and can be used
        for (size_t i = 0; i < tasks_length; i++) {
            if (tasks[i] == NULL) {
                tasks[i] = task;
                task->pid = i;
                tasks_present++;
                break;
            }
        }
    }

    return task;
}

task_t* task_create(const char name[64], void (*entrypoint)(), bool is_kernel)
{
    task_t* task = task_allocate(name);

    // Allocate kernel stack (required for usermode tasks as well)
    task->kernel_stack_size = KERNEL_STACK_SIZE;
    task->kernel_stack_base = (uint32_t)(vmm_kernel_alloc(KERNEL_STACK_SIZE));
    task_setup_initial_stack(task, entrypoint, is_kernel);

    // Set initial state
    task->state = TASK_WAITING;

    if (task->kernel_stack_base == 0) {
        // task_destroy(task);
        // return NULL;
        abort();
    }

    // For now we're uisng kernel page directory for all tasks
    // TODO: create new page directories for each task
    task->page_dir_virt = paging_get_kernel_directory_virt();
    task->page_dir_phys = paging_get_kernel_directory_phys();


    return task;
}

void task_destroy(task_t* task)
{
    // Free task entry
    tasks[task->pid] = NULL;
    tasks_present--;

    // Dellocate memory for task
    kfree(task);
}
