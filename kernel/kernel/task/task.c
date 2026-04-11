#include <kernel/task/task.h>
#include "kernel/memory/pmm.h"
#include "kernel/spinlock.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include <kernel/task/scheduler.h>
#include <kernel/memory/vmm.h>
#include "kernel/debug.h"
#include <stdlib.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/paging.h>
#include <kernel/elf.h>
#include <string.h>

#define _debug_puts(...) debug_puts_c("Task", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("Task", __VA_ARGS__)

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
#define MIN_HEAP_STACK_GAP (1 * 1024 * 1024) // 1 MB

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
    bool is_kernel,
    uint32_t user_esp
) {
    // Get top of the kernel stack. Stack grows downward, so we push in reverse
    // order.
    uint32_t* stack = (uint32_t*)(
        task->kernel_stack_base + task->kernel_stack_size
    );

    if (!is_kernel) {
        // User mode task - need SS:ESP for IRET
        if (!user_esp) {
            // New task: push trampoline so that when main() returns, we exit cleanly
            stack--; *stack = (uint32_t)task_entry_trampoline;
        }
        stack--;
        *stack = 0x23; // User data segment (GDT entry 4, RPL=3)
        stack--;
        *stack = user_esp ? user_esp : task->user_stack_base + task->user_stack_size;
    } else {
        stack--; *stack = (uint32_t)task_entry_trampoline; // Return address
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

    // Segment registers, saved by interrupt handler (popped last by task_switch_return_point)
    if (is_kernel) {
        stack--; *stack = 0x10; // GS (kernel data)
        stack--; *stack = 0x10; // FS
        stack--; *stack = 0x10; // ES
        stack--; *stack = 0x10; // DS
    } else {
        stack--; *stack = 0x23; // GS (user data, RPL=3)
        stack--; *stack = 0x23; // FS
        stack--; *stack = 0x23; // ES
        stack--; *stack = 0x23; // DS
    }

    // Registers saved by PUSHA (popped first by task_switch_return_point via popal)
    stack--; *stack = 0; // EAX
    stack--; *stack = 0; // ECX
    stack--; *stack = 0; // EDX
    stack--; *stack = 0; // EBX
    stack--; *stack = 0; // ESP placeholder, ignored by POPA
    stack--; *stack = 0; // EBP
    stack--; *stack = 0; // ESI
    stack--; *stack = 0; // EDI

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

    task->fd = NULL;
    task->fd_count = 0;
    task->waiting_queue = wait_allocate_event();
    task->children_wait_queue = wait_allocate_event();

    if (tasks_length == tasks_present) {
        // Need to increase the size of tasks array by some constant margin
        uint32_t new_size = tasks_length + TASKS_LIST_REALLOC_SIZE;
        tasks = krealloc(tasks, sizeof(task_t*) * new_size);

        // Clear newly allocated memory to zero it out, which can be used later
        // to check if entry is empty (NULL) or not
        memset(
            tasks + tasks_length,
            0,
            sizeof(task_t*) * TASKS_LIST_REALLOC_SIZE
        );
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

    task_setup_initial_stack(task, entrypoint, is_kernel, 0);

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
        _debug_printf("Failed to clone page directory for task %s\n", name);
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

    // Free task descriptors
    for (size_t i = 0; i < task->fd_count; i++) {
        task_file_descriptor_t* fd = task->fd[i];
        if (fd->file != NULL) {
            vfs_close(fd->file);
        }
        kfree(fd);
    }
    kfree(task->fd);

    // Free queues
    wait_deallocate(task->waiting_queue);
    wait_deallocate(task->children_wait_queue);

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
        _debug_puts("ERROR: task_exit called but no task is running.");
        abort();
    }

    _debug_printf(
        "Task [%u] '%s' exited with code %d\n",
        current->pid,
        current->name,
        exit_code
    );

    // Mark task as terminated and update exit code
    current->exit_code = exit_code;
    scheduler_move_task_to_state(current, TASK_ZOMBIE);

    // Notify all tasks waiting for this process to be over
    wait_wake_up(current->waiting_queue);
    // If parent is currently waiting for any child process, wake it up as well
    if (current->parent != NULL) {
        wait_wake_up(current->parent->children_wait_queue);
    }

    // TODO: emit SIGCHLD if parent task exists

    // Handle cases where current task has children
    task_t* child = current->first_child;
    while (child != NULL) {
        task_t* next_child = child->next_sibling;
        if (child->state == TASK_ZOMBIE) {
            // Child is already zombie, no one will wait for it.
            scheduler_move_task_to_state(child, TASK_DEAD);
            _debug_printf(
                "Orphaned zombie [%u] '%s' marked as DEAD\n",
                child->pid,
                child->name
            );
        } else {
            // Child is still running - reparenting them to current task's
            // original parent
            child->parent = current->parent;
            if (child->parent == NULL) {
                _debug_printf(
                    "Child [%u] '%s' orphaned\n",
                    child->pid,
                    child->name
                );
            } else {
                _debug_printf(
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

typedef struct {
    int pid;
    int* exit_code;
} task_wait_on_child_params;

// Pointer to a child found in a zombie state for task_wait_for_any_child.
static task_t* any_child_zombie_found = NULL;

/**
 * Will return true if the child with given PID is in ZOMBIE state. It will
 * store exit code in passed address, if given.
 */
static bool task_wait_for_child(void* _params)
{
    task_wait_on_child_params* params = _params;
    int pid = params->pid;
    int* exit_code = params->exit_code;

    task_t* parent = scheduler_get_current_task();
    if (parent == NULL) {
        return false;
    }
    task_t* child = parent->first_child;
    while (child != NULL) {
        if (child->pid != (uint32_t)pid) {
            child = child->next_sibling;
            continue;
        }

        // Child found
        if (child->state != TASK_ZOMBIE) {
            return false;
        }

        if (exit_code != NULL) {
            *exit_code = child->exit_code;
        }
        return true;
    }
    // Child not found
    return false;
}

/**
 * Will return true if any child with is in ZOMBIE state. It will
 * store exit code in passed address, if given. "pid" parameter is ignored.
 */
static bool task_wait_for_any_child(void* _params)
{
    task_wait_on_child_params* params = _params;
    int* exit_code = params->exit_code;

    task_t* parent = scheduler_get_current_task();
    if (parent == NULL) {
        return false;
    }
    task_t* child = parent->first_child;
    while (child != NULL && child->state != TASK_ZOMBIE) {
        child = child->next_sibling;
    }
    if (child == NULL) {
        // Child not found
        return false;
    }

    // Child found - save exit code and return
    if (exit_code != NULL) {
        *exit_code = child->exit_code;
    }
    any_child_zombie_found = child;
    return true;
}

void task_save_syscall_user_context(uint32_t* syscall_frame)
{
    // syscall_frame points to [EAX, ECX, EDX, EBX, EBP, ESI, EDI,
    //                          user_EIP, CS, EFLAGS, user_ESP, SS]
    task_t* current = scheduler_get_current_task();
    if (current) {
        current->context.syscall_user_eax = syscall_frame[0];
        current->context.syscall_user_ecx = syscall_frame[1];
        current->context.syscall_user_edx = syscall_frame[2];
        current->context.syscall_user_ebx = syscall_frame[3];
        current->context.syscall_user_ebp = syscall_frame[4];
        current->context.syscall_user_esi = syscall_frame[5];
        current->context.syscall_user_edi = syscall_frame[6];
        current->context.syscall_user_eip = syscall_frame[7];
        current->context.syscall_user_esp = syscall_frame[10];
        current->context.syscall_frame_ptr = (uint32_t)syscall_frame;
    }
}

void task_set_fork_child_return(task_t* child)
{
    uint32_t* saved_eax = (uint32_t*)(child->context.esp + 36);
    *saved_eax = 0;
}

task_t* task_fork(void)
{
    task_t* parent = scheduler_get_current_task();
    task_t* child = task_allocate(parent->name);

    // Copy user-space layout and heap bounds from parent
    child->user_stack_base = parent->user_stack_base;
    child->user_stack_size = parent->user_stack_size;
    child->heap_start = parent->heap_start;
    child->heap_end = parent->heap_end;
    child->heap_max = parent->heap_max;

    // Duplicate memory-region list
    task_memory_region_t* tmr = parent->memory_regions;
    while (tmr != NULL) {
        task_add_mem_region(child, tmr->start, tmr->end, tmr->page_flags);
        tmr = tmr->next;
    }

    // Allocate child kernel stack (same guard-page strategy as task_create)
    void* allocation = vmm_kernel_alloc(KERNEL_STACK_SIZE + GUARD_PAGE_SIZE);
    if (allocation == NULL) {
        abort();
    }
    uint32_t guard_phys = paging_get_physical_address((uint32_t)allocation);
    paging_unmap_page((uint32_t)allocation);
    if (guard_phys != 0) {
        pmm_free_page(guard_phys);
    }
    child->kernel_stack_size = KERNEL_STACK_SIZE;
    child->kernel_stack_base = (uint32_t)allocation + GUARD_PAGE_SIZE;
    child->kernel_stack_alloc_size = KERNEL_STACK_SIZE + GUARD_PAGE_SIZE;
    child->kernel_stack_alloc_base = (uint32_t)allocation;

    // Use user-mode EIP/ESP saved at syscall entry by task_save_syscall_user_context.
    uint32_t fork_return_eip = parent->context.syscall_user_eip;
    uint32_t fork_return_esp = parent->context.syscall_user_esp;
    _debug_printf("FORK: parent eip=%x esp=%x\n", fork_return_eip, fork_return_esp);

    // Build a fresh initial stack for the child that will IRET back to the
    // instruction after the fork() syscall in user space, with EAX=0.
    task_setup_initial_stack(child, (void(*)())fork_return_eip, false, fork_return_esp);

    // Patch the child's PUSHA frame with the parent's register state so the
    // child resumes with the same callee-saved registers the parent had at the
    // fork() syscall.  EAX stays 0 (fork return value for child).
    //
    // Stack layout from context.esp (uint32_t offsets):
    //   [0..3] switch_to_stack callee-saved (EDI, ESI, EBX, EBP)
    //   [4]    return addr (task_switch_return_point)
    //   [5..12] PUSHA frame: EDI, ESI, EBP, ESP(ignored), EBX, EDX, ECX, EAX
    uint32_t* child_stack = (uint32_t*)child->context.esp;
    child_stack[5]  = parent->context.syscall_user_edi;
    child_stack[6]  = parent->context.syscall_user_esi;
    child_stack[7]  = parent->context.syscall_user_ebp;
    child_stack[9]  = parent->context.syscall_user_ebx;
    child_stack[10] = parent->context.syscall_user_edx;
    child_stack[11] = parent->context.syscall_user_ecx;

    // Clone parent's page directory with COW for user space
    void* child_pd = paging_clone_directory(
        parent->page_dir_virt,
        true,
        child->user_stack_base,
        child->user_stack_size
    );
    if (child_pd == NULL) {
        vmm_kernel_free(
            (void*)child->kernel_stack_alloc_base,
            child->kernel_stack_alloc_size
        );
        tasks[child->pid] = NULL;
        tasks_present--;
        kfree(child);
        return NULL;
    }
    child->page_dir_virt = child_pd;
    child->page_dir_phys = paging_virt_to_phys(child_pd);

    // Duplicate open file descriptors
    if (parent->fd_count > 0) {
        child->fd = kmalloc(sizeof(task_file_descriptor_t*) * parent->fd_count);
        child->fd_count = parent->fd_count;
        for (size_t i = 0; i < parent->fd_count; i++) {
            task_file_descriptor_t* pfd = parent->fd[i];
            task_file_descriptor_t* cfd = kmalloc(
                sizeof(task_file_descriptor_t)
            );
            cfd->identifier = pfd->identifier;
            if (pfd->file == NULL) {
                cfd->file = NULL;
            } else {
                uint8_t mode;
                if (pfd->file->readable && pfd->file->writeable) {
                    mode = VFS_MODE_READWRITE;
                } else if (pfd->file->writeable) {
                    mode = VFS_MODE_WRITEONLY;
                } else {
                    mode = VFS_MODE_READONLY;
                }
                cfd->file = vfs_open(pfd->file->node, mode);
                cfd->file->offset = pfd->file->offset;
            }
            child->fd[i] = cfd;
        }
    }

    // Child sees 0 as the fork() return value (EAX=0 is already in the fresh stack)

    // Wire into process hierarchy
    child->parent = parent;
    if (parent->first_child == NULL) {
        parent->first_child = child;
    } else {
        task_t* last = parent->first_child;
        while (last->next_sibling != NULL) {
            last = last->next_sibling;
        }
        last->next_sibling = child;
        child->prev_sibling = last;
    }

    child->state = TASK_WAITING;
    scheduler_add_task(child);

    return child;
}

int task_wait(int pid, int* exit_code)
{
    task_t* parent = scheduler_get_current_task();
    if (parent->first_child == NULL) {
        // Parent doesn't have children
        return -1;
    }
    // Check if parent has a child with that PID
    if (pid >= 0) {
        task_t* child = parent->first_child;
        while (child != NULL) {
            if (child->pid == (uint32_t)pid) {
                break;
            }
            child = child->next_sibling;
        }
        if (child == NULL) {
            // Parent doesn't have a child with that PID
            return -2;
        }

        // Parent has a child with given PID - we can register it in the waiting
        // queue.

        task_wait_on_child_params* params = kmalloc(
            sizeof(task_wait_on_child_params)
        );
        params->pid = pid;
        params->exit_code = exit_code;
        wait_on_condition(
            child->waiting_queue,
            task_wait_for_child,
            (void*)params
        );
        kfree(params);
        if (exit_code != NULL) {
            *exit_code = child->exit_code;
        }
        child->state = TASK_DEAD;
        return child->pid;
    }

    // Parent wants to wait for any child to be completed.
    task_wait_on_child_params* params = kmalloc(
        sizeof(task_wait_on_child_params)
    );
    params->exit_code = exit_code;
    wait_on_condition(
        parent->children_wait_queue,
        task_wait_for_any_child,
        (void*)params
    );
    kfree(params);
    if (exit_code != NULL) {
        *exit_code = any_child_zombie_found->exit_code;
    }
    any_child_zombie_found->state = TASK_DEAD;
    return any_child_zombie_found->pid;
}

int task_exec(
    void* elf_data,
    size_t elf_size,
    int argc,
    const char** argv,
    int envc,
    const char** envp
) {
    (void)elf_size;

    elf_t* parsed = kmalloc(sizeof(elf_t));
    elf_parse(elf_data, parsed);
    if (!parsed->success) {
        kfree(parsed);
        return -1;
    }

    task_t* current = scheduler_get_current_task();

    void* old_pd_virt = current->page_dir_virt;
    uint32_t old_pd_phys = current->page_dir_phys;

    // Create a fresh page directory: kernel mappings + new user stack (COW)
    void* new_pd = paging_clone_directory(
        paging_get_kernel_directory(),
        true,
        current->user_stack_base,
        current->user_stack_size
    );
    if (new_pd == NULL) {
        kfree(parsed);
        return -1;
    }

    // Switch to the new page directory and load ELF segments into it
    paging_switch_directory(new_pd);

    if (!elf_load_segments(elf_data, parsed)) {
        // Loading failed: restore old address space and discard new one
        paging_switch_directory(old_pd_virt);
        paging_free_user_pages(new_pd);
        pmm_free_page(paging_virt_to_phys(new_pd));
        kfree(parsed);
        return -1;
    }

    // Commit: free the old address space (we are already running on new_pd)
    paging_free_user_pages(old_pd_virt);
    pmm_free_page(old_pd_phys);

    current->page_dir_virt = new_pd;
    current->page_dir_phys = paging_virt_to_phys(new_pd);

    // Rebuild memory region list from new ELF segments
    task_memory_region_t* memreg = current->memory_regions;
    while (memreg != NULL) {
        task_memory_region_t* next = memreg->next;
        kfree(memreg);
        memreg = next;
    }
    current->memory_regions = NULL;
    for (size_t i = 0; i < parsed->segment_count; i++) {
        elf_segment_t* seg = &parsed->segments[i];
        task_add_mem_region(
            current,
            PAGE_ALIGN_DOWN(seg->vaddr),
            PAGE_ALIGN_UP(seg->vaddr + seg->memsz),
            seg->page_flags
        );
    }

    // Reset heap to start right after the highest loaded address
    uint32_t heap_start = PAGE_ALIGN_UP(parsed->max_vaddr);
    current->heap_start = heap_start;
    current->heap_end = heap_start;
    current->heap_max = current->user_stack_base - MIN_HEAP_STACK_GAP;

    // Patch the live IRET frame on the kernel stack so that the iret executed
    // by the syscall ISR on return will jump to the new entry point with a
    // fresh user stack, instead of back into the now-destroyed old image.
    // Frame layout (set by task_save_syscall_user_context):
    //   [7] = user EIP, [10] = user ESP
    uint32_t* frame = (uint32_t*)current->context.syscall_frame_ptr;
    frame[7]  = parsed->entry_point;
    frame[10] = current->user_stack_base + current->user_stack_size;

    /* Write argc/argv/envp onto the new user stack (page directory is already
     * switched, so writes reach the new address space directly). */
    task_push_args(&frame[10], argc, argv, envc, envp);

    kfree(parsed);
    return 0;
}
