#include "kernel/task/task.h"
#include "kernel/memory/kmalloc.h"
#include <string.h>
#include <stdint.h>

/**
 * Write argc/argv/envp onto the user stack following the System V i386 ABI.
 *
 * Stack layout produced (low → high addresses, ESP points at argc on exit):
 *
 *   [argc]
 *   [argv[0]] ... [argv[argc-1]] [NULL]
 *   [envp[0]] ... [envp[envc-1]] [NULL]
 *   <arg strings, null-terminated>
 *   <env strings, null-terminated>   ← original *esp_inout
 *
 * The task's page directory must be active when this is called so that
 * writes to user-space addresses are visible.
 *
 * argv/envp may be NULL (treated as empty).
 */
void task_push_args(
    uint32_t* esp_inout,
    int argc, const char** argv,
    int envc, const char** envp
) {
    uint8_t* sp = (uint8_t*)(*esp_inout);

    /* Temporary kernel-side storage for the user-space string pointers we are
     * about to write so we can build the pointer arrays afterwards. */
    uint32_t* argv_ptrs = argc > 0
        ? kmalloc(sizeof(uint32_t) * (size_t)argc)
        : NULL;
    uint32_t* envp_ptrs = envc > 0
        ? kmalloc(sizeof(uint32_t) * (size_t)envc)
        : NULL;

    /* Copy env strings onto the user stack (highest index first so that
     * lower indices end up closer to the pointer array). */
    for (int i = envc - 1; i >= 0; i--) {
        size_t len = strlen(envp[i]) + 1;
        sp -= len;
        memcpy(sp, envp[i], len);
        envp_ptrs[i] = (uint32_t)sp;
    }

    /* Copy arg strings onto the user stack. */
    for (int i = argc - 1; i >= 0; i--) {
        size_t len = strlen(argv[i]) + 1;
        sp -= len;
        memcpy(sp, argv[i], len);
        argv_ptrs[i] = (uint32_t)sp;
    }

    /* Align the stack pointer down to a 4-byte boundary. */
    sp = (uint8_t*)((uint32_t)sp & ~3U);

    /* Push NULL-terminated envp pointer array (NULL sentinel first so it ends
     * up at the highest address, then pointers from last to first). */
    sp -= 4;
    *(uint32_t*)sp = 0; /* envp[envc] = NULL */
    for (int i = envc - 1; i >= 0; i--) {
        sp -= 4;
        *(uint32_t*)sp = envp_ptrs[i];
    }

    /* Push NULL-terminated argv pointer array. */
    sp -= 4;
    *(uint32_t*)sp = 0; /* argv[argc] = NULL */
    for (int i = argc - 1; i >= 0; i--) {
        sp -= 4;
        *(uint32_t*)sp = argv_ptrs[i];
    }

    /* Push argc. */
    sp -= 4;
    *(uint32_t*)sp = (uint32_t)argc;

    if (argv_ptrs) kfree(argv_ptrs);
    if (envp_ptrs) kfree(envp_ptrs);

    *esp_inout = (uint32_t)sp;
}
