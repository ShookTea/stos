#include <kernel/task/task.h>
#include "kernel/memory/pmm.h"
#include "kernel/spinlock.h"
#include <kernel/task/scheduler.h>
#include <kernel/memory/vmm.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/paging.h>
#include <string.h>

/** When increasing tasks list length, use this size */
#define TASKS_LIST_REALLOC_SIZE 10

static task_t** tasks = NULL;
// Length of tasks list
static size_t tasks_length = 0;
// The number of tasks that weren't destroyed yet
static size_t tasks_present = 0;
static spinlock_t task_list_lock = SPINLOCK_INIT;

#define KERNEL_STACK_SIZE (16 * 1024) // 16 KiB
#define GUARD_PAGE_SIZE (4 * 1024)
#define USER_STACK_SIZE (8 * 1024) // 8 KiB
#define USER_STACK_TOP (VMM_USER_END - PAGE_SIZE)
#define USER_STACK_BASE (USER_STACK_TOP - USER_STACK_SIZE)

/**
 * This function is never called directly, but it's a return address for a task.
 * When task's real function returns, execution will continue here.
 */
static void task_entry_trampoline()
{
    // The entry function's return value is in EAX.
    int exit_code;
    __asm__ volatile("mov %%eax, %0" : "=r"(exit_code));
    task_exit(exit_code);
}

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

    stack--; *stack = (uint32_t)task_entry_trampoline; // Return address

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

    // Fake return address for the call chain - this will make the "ret"
    // instruction from switch_to_stack work correctly
    extern void task_switch_return_point(void);
    stack--; *stack = (uint32_t)task_switch_return_point;

    // Callee-saved registers (for switch_to_stack)
    stack--; *stack = 0; // EBP
    stack--; *stack = 0; // EBX
    stack--; *stack = 0; // ESI
    stack--; *stack = 0; // EDI

    // Save final ESP to task context
    task->context.esp = (uint32_t)stack;
}

/**
 * Allocate new task in the tasks array, with given name, and set PID. Grow
 * tasks array if necessary; otherwise reuse the already existing entries.
 */
static task_t* task_allocate(const char* name)
{
    spinlock_acquire(&task_list_lock);
    // Allocate task data in memory
    task_t* task = kmalloc_flags(sizeof(task_t), KMALLOC_ZERO);
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0'; // Ensure null termination

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

    spinlock_release(&task_list_lock);
    return task;
}

task_t* task_create(const char* name, void (*entrypoint)(), bool is_kernel)
{
    task_t* task = task_allocate(name);

    // Allocate kernel stack (required for usermode tasks as well), with
    // guard page (so that when task overflows its stack, we will hit the guard
    // page instead of corrupting other memory)
    //
    // Strategy: Allocate the entire region (guard + stack) to ensure they're
    // contiguous, then free the physical page backing the guard page while
    // keeping the virtual address mapped (but invalid).

    void* allocation = vmm_kernel_alloc(KERNEL_STACK_SIZE + GUARD_PAGE_SIZE);
    if (allocation == NULL) {
        // task_destroy(task);
        // return NULL;
        abort();
    }

    // Get the physical address of the guard page before unmapping
    uint32_t guard_phys = paging_get_physical_address((uint32_t)allocation);

    // Unmap the guard page (removes PTE)
    paging_unmap_page((uint32_t)allocation);

    // Free the physical page that was backing the guard page
    if (guard_phys != 0) {
        pmm_free_page(guard_phys);
    }

    // Now the guard page virt. address is reserved but has no physical backing
    task->kernel_stack_size = KERNEL_STACK_SIZE;
    task->kernel_stack_base = (uint32_t)allocation + GUARD_PAGE_SIZE;
    task->kernel_stack_alloc_size = KERNEL_STACK_SIZE + GUARD_PAGE_SIZE;
    task->kernel_stack_alloc_base = (uint32_t)allocation;

    // Allocate user stack for usermode tasks
    if (is_kernel) {
        task->user_stack_base = 0;
        task->user_stack_size = 0;
    } else {
        task->user_stack_size = USER_STACK_SIZE;
        task->user_stack_base = USER_STACK_BASE;
        // Physical pages will be allocated by page directory cloning with COW
    }

    task_setup_initial_stack(task, entrypoint, is_kernel);

    // Set initial state
    task->state = TASK_WAITING;

    // Clone page directory
    // For kernel tasks: only kernel mappings
    // For user tasks: kernel + user space with COW
    void* kernel_directory = paging_get_kernel_directory();
    void* kernel_dir_clone = paging_clone_directory(
        kernel_directory,
        !is_kernel,
        task->user_stack_base,
        task->user_stack_size
    );
    if (kernel_dir_clone == NULL) {
        printf("TASK: Failed to clone page directory for task %s\n", name);
        vmm_kernel_free(
            (void*)task->kernel_stack_alloc_base,
            task->kernel_stack_alloc_size
        );
        tasks[task->pid] = NULL;
        tasks_present--;
        kfree(task);
        abort(); // TODO: is abort needed here?
        return NULL;
    }
    task->page_dir_virt = kernel_dir_clone;
    task->page_dir_phys = paging_virt_to_phys(kernel_dir_clone);

    // Set parent
    task_t* parent = scheduler_get_current_task();
    if (parent == NULL) {
        task->parent = NULL;
    } else {
        task->parent = parent;
        if (parent->first_child == NULL) {
            parent->first_child = task;
        } else {
            task_t* last = parent->first_child;
            while (last->next_sibling != NULL) {
                last = last->next_sibling;
            }
            last->next_sibling = task;
            task->prev_sibling = last;
        }
    }

    return task;
}

// TODO:
// - close open file descriptiors
// - free user-space memory (but keep kernel stack)
void task_destroy(task_t* task)
{
    if (task == NULL){
        return;
    }

    // Free kernel stack, which is always allocated, even for usermode tasks
    // Note: guard page's physical memory was already freed during creation,
    // so vmm_kernel_free will only try to free the stack pages. The guard page
    // virtual address will be freed as part of the contiguous region.
    if (task->kernel_stack_alloc_base != 0) {
        vmm_kernel_free(
            (void*)task->kernel_stack_alloc_base,
            task->kernel_stack_alloc_size
        );
        task->kernel_stack_base = 0;
        task->kernel_stack_size = 0;
        task->kernel_stack_alloc_base = 0;
        task->kernel_stack_alloc_size = 0;
    }

    // Free user stack (if allocated)
    if (task->user_stack_base != 0) {
        // TODO: implement this when we have per-process page directories
        task->user_stack_base = 0;
        task->user_stack_size = 0;
    }

    if (task->page_dir_virt != NULL &&
        task->page_dir_virt != paging_get_kernel_directory()) {
        paging_free_user_pages(task->page_dir_virt);
        pmm_free_page(task->page_dir_phys);
        task->page_dir_virt = NULL;
        task->page_dir_phys = 0;
    }

    // Free memory regions list
    task_memory_region_t* memreg = task->memory_regions;
    while (memreg != NULL) {
        task_memory_region_t* next = memreg->next;
        kfree(memreg);
        memreg = next;
    }
    task->memory_regions = NULL;

    // Free task entry
    spinlock_acquire(&task_list_lock);
    tasks[task->pid] = NULL;
    tasks_present--;
    spinlock_release(&task_list_lock);

    // Dellocate memory for task
    kfree(task);
}

size_t task_get_tasks_count()
{
    return tasks_present;
}

task_t* task_get_task_by_index(size_t index)
{
    if (index >= tasks_present) {
        return NULL;
    }
    spinlock_acquire(&task_list_lock);
    size_t found = 0;
    for (size_t i = 0; i < tasks_length; i++) {
        if (tasks[i] != NULL) {
            if (found == index) {
                spinlock_release(&task_list_lock);
                return tasks[i];
            }
            found++;
        }
    }
    spinlock_release(&task_list_lock);
    return NULL;
}

/**
 * Returns task by PID
 */
task_t* task_get_task_by_pid(uint32_t pid)
{
    spinlock_acquire(&task_list_lock);
    for (size_t i = 0; i < tasks_length; i++) {
        if (tasks[i] != NULL && tasks[i]->pid == pid) {
            spinlock_release(&task_list_lock);
            return tasks[i];
        }
    }
    spinlock_release(&task_list_lock);
    return NULL;
}

void task_add_mem_region(
    task_t* task,
    uint32_t start,
    uint32_t end,
    uint32_t page_flags
) {
    task_memory_region_t* region = kmalloc(sizeof(task_memory_region_t));
    region->start = start;
    region->end = end;
    region->page_flags = page_flags;
    region->next = NULL;

    // Add to linked list
    if (task->memory_regions == NULL) {
        task->memory_regions = region;
    } else {
        task_memory_region_t* last = task->memory_regions;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = region;
    }
}

__attribute__((noreturn))
void task_exit(int exit_code)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        // This should never happen. It should probably be handled gracefully,
        // like this:
        // while (1) {
        //     __asm__ volatile("hlt");
        // }
        // but for now, for sake of debugging, let's print some info instead.
        puts("ERROR: task_exit called but no task is running.");
        abort();
    }

    printf(
        "Task [%u] '%s' exited with code %d\n",
        current->pid,
        current->name,
        exit_code
    );

    // Mark task as terminated and update exit code
    current->exit_code = exit_code;
    scheduler_move_task_to_state(current, TASK_ZOMBIE);

    // Wake up parent if it's waiting
    // TODO: emit SIGCHLD as well
    if (current->parent != NULL && current->parent->state == TASK_BLOCKED) {
        scheduler_move_task_to_state(current->parent, TASK_WAITING);
    }

    // Handle cases where current task has children
    task_t* child = current->first_child;
    while (child != NULL) {
        task_t* next_child = child->next_sibling;
        if (next_child->state == TASK_ZOMBIE) {
            // Child is already zombie, no one will wait for it.
            scheduler_move_task_to_state(child, TASK_DEAD);
            printf(
                "Orphaned zombie [%u] '%s' marked as DEAD\n",
                child->pid,
                child->name
            );
        } else {
            // Child is still running - reparenting them to current task's
            // original parent
            child->parent = current->parent;
            if (child->parent == NULL) {
                printf(
                    "Child [%u] '%s' orphaned\n",
                    child->pid,
                    child->name
                );
            } else {
                printf(
                    "Child [%u] '%s' changed parent to [%u] '%s'\n",
                    child->pid,
                    child->name,
                    child->parent->pid,
                    child->parent->name
                );
            }
        }
        child = next_child;
    }

    scheduler_yield();
    // We should never enter this part of code
    while (1) {}
}

int task_wait(int pid, int* exit_code)
{
    task_t* parent = scheduler_get_current_task();
    while (1) {
        // Look for zombie children
        task_t* child = parent->first_child;

        while (child != NULL) {
            // If waiting for specific PID, check it
            if (pid >= 0 && child->pid != (uint32_t)pid) {
                child = child->next_sibling;
                continue;
            }

            if (child->state == TASK_ZOMBIE) {
                // Found zombie child to reap.
                // Read exit code if requested
                if (exit_code != NULL) {
                    *exit_code = child->exit_code;
                }

                int child_pid = child->pid;

                // Remove from parent's child list
                if (child->prev_sibling != NULL) {
                    child->prev_sibling->next_sibling = child->next_sibling;
                }
                if (child->next_sibling != NULL) {
                    child->next_sibling->prev_sibling = child->prev_sibling;
                }
                if (parent->first_child == child) {
                    parent->first_child = child->next_sibling;
                }

                // Mark child as DEAD (to be cleaned up by the scheduler)
                child->state = TASK_DEAD;
                return child_pid;
            }
            child = child->next_sibling;
        }

        // No zombie children found

        if (parent->first_child == NULL) {
            // Parent doesn't have any children
            return -1;
        }

        // Children exist, but none are zombies yet. Block and wait for a child
        // to exit.
        scheduler_move_task_to_state(parent, TASK_BLOCKED);
        scheduler_yield();
    }
}
