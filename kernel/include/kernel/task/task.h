#ifndef KERNEL_TASK_TASK_H
#define KERNEL_TASK_TASK_H

#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Process states enum
 */
typedef enum {
    /** Waiting to be run by the scheduler */
    TASK_WAITING = 1,
    /** Currently running task */
    TASK_RUNNING = 2,
    /** Waiting for external event (I/O reads, user inputs, etc.) */
    TASK_BLOCKED = 3,
    /** Sleeping for some timeout */
    TASK_SLEEPING = 4,
    /** No longer running, but waits for parent to read the exit status */
    TASK_ZOMBIE = 5,
    /** Fully terminated task, ready for cleanup */
    TASK_DEAD = 6,
} task_state_t;

/**
 * CPU context for task switching. Because we use stack-based context switching,
 * we only need to store ESP - stack pointer itself.
 */
typedef struct {
    uint32_t esp;
    /** User-mode EIP saved at syscall entry (used by fork) */
    uint32_t syscall_user_eip;
    /** User-mode ESP saved at syscall entry (used by fork) */
    uint32_t syscall_user_esp;
    /** GP registers saved at syscall entry (used by fork to clone parent state) */
    uint32_t syscall_user_eax;
    uint32_t syscall_user_ecx;
    uint32_t syscall_user_edx;
    uint32_t syscall_user_ebx;
    uint32_t syscall_user_ebp;
    uint32_t syscall_user_esi;
    uint32_t syscall_user_edi;
} task_cpu_context_t;

/**
 * Memory region structure - represents a region of virtual memory with specific
 * properties
 */
typedef struct memory_region {
    /** Start virtual address (inclusive) */
    uint32_t start;
    /** End virtual address (exclusive) */
    uint32_t end;
    /** Page flags (defined in paging.h) */
    uint32_t page_flags;
    /** Next region in the linked list */
    struct memory_region* next;
} task_memory_region_t;

/**
 * File descriptor list
 */
typedef struct {
    vfs_file_t* file; // set to NULL if not present
    // FD identifier, starting from 3 (0-2 are reserved for stdin/out/err)
    uint32_t identifier;
} task_file_descriptor_t;

/**
 * Task Control Block (TCB) - defines all information needed to manage and
 * swtich between tasks
 */
typedef struct task {
    // Process ID and state
    /** Process ID (unique) */
    uint32_t pid;
    /** Process name */
    char name[64];
    /** Current state of the task */
    task_state_t state;
    /** Exit code (valid when state = TASK_ZOMBIE) */
    int exit_code;

    // CPU context and memory management
    /** Full CPU context */
    task_cpu_context_t context;
    /** Physical address of task's page directory */
    uint32_t page_dir_phys;
    /** Virtual address of task's page directory */
    void* page_dir_virt;
    /** Linked list of memory regions */
    task_memory_region_t* memory_regions;

    // Stack and heap information
    /** Base of kernel stack (high address) */
    uint32_t kernel_stack_base;
    /** Size of the kernel stack */
    uint32_t kernel_stack_size;
    /** The actual allocation address, including guard page */
    uint32_t kernel_stack_alloc_base;
    /** The actual allocation size, including guard page */
    uint32_t kernel_stack_alloc_size;
    /** Base of user stack (high address) */
    uint32_t user_stack_base;
    /** Size of the user stacvk */
    uint32_t user_stack_size;
    /** Start of the heap region */
    uint32_t heap_start;
    /** Current heap break point */
    uint32_t heap_end;
    /** Maximum allowed heap size */
    uint32_t heap_max;

    // Process hierarchy
    /** Parent process */
    struct task* parent;
    /** First child in linked list, navigable by next_sibling */
    struct task* first_child;
    /** Previous sibling in linked list of children */
    struct task* prev_sibling;
    /** Next sibling in linked list of children */
    struct task* next_sibling;

    // Scheduling data
    /** Remaining time slice (in PIT ticks) */
    uint32_t time_slice;
    /** Total CPU time used (in PIT ticks) */
    uint64_t total_runtime;

    // Linked list for scheduler queues
    /** Previous task in queue (for double-direction linked list) */
    struct task* prev;
    /** Next task in the queue */
    struct task* next;

    // Open file descriptors + total count
    task_file_descriptor_t** fd;
    size_t fd_count;

    /** Waiting queue for processes that wait for this task to be completed */
    wait_obj_t* waiting_queue;
    /** Special queue for parent that waits for any children to complete */
    wait_obj_t* children_wait_queue;

    // TODO: for future implementations:
    // - priority
    // - working directory ID/path
    // - pending signals to be sent to the process
    // - signal handlers
} task_t;

/**
 * Create a new task with given name, entrypoint, and permission ring
 */
task_t* task_create(const char* name, void (*entry_point)(), bool is_kernel);

/**
 * Deallocates all memory from a task, fixes linked lists if necessary
 */
void task_destroy(task_t* task);

/**
 * Returns total number of all tasks
 */
size_t task_get_tasks_count();

/**
 * Returns N-th existing task on the list
 */
task_t* task_get_task_by_index(size_t index);

/**
 * Returns task by PID
 */
task_t* task_get_task_by_pid(uint32_t pid);

/**
 * Registers new memory region for the task.
 */
void task_add_mem_region(
    task_t* task,
    uint32_t start,
    uint32_t end,
    uint32_t page_flags
);

/**
 * Exit the currently running task with given exit code. Code = 0 is interpreted
 * as success, any other code is interpreted as an error.
 */
__attribute__((noreturn))
void task_exit(int exit_code);

/**
 * Wait for a child task to exit and reap it.
 * - "pid" is the PID of child to wait for, or -1 for any child
 * - "exit_code" is the pointer where exit code will be stored (can be NULL)
 * - returns PID of reaped child, or -1 on error.
 */
int task_wait(int pid, int* exit_code);

/**
 * Patch the child task's saved EAX in its kernel-stack interrupt frame to 0,
 * so that when the child is scheduled it returns 0 from fork() while the
 * parent receives the child's PID.  Must be called after the child's kernel
 * stack has been cloned and before the child is enqueued by the scheduler.
 */
void task_set_fork_child_return(task_t* child);

/**
 * Save the user-mode registers from the syscall entry frame into the current
 * task's context. Called from syscall_stub with the kernel ESP at the point
 * where all 7 GP registers (EAX/ECX/EDX/EBX/EBP/ESI/EDI) have been pushed,
 * so the IRET frame is at known offsets above that pointer.
 */
void task_save_syscall_user_context(uint32_t* syscall_frame);

/**
 * Fork the current task. Clones the kernel stack, page directory (COW), and
 * open file descriptors. The child is queued immediately; its fork() return
 * value is patched to 0. Returns the new child task_t* (caller reads
 * child->pid to return to the parent), or NULL on failure.
 */
task_t* task_fork(void);

#endif
