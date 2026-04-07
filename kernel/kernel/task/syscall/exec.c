#include "../syscall.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/memory/vmm.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <string.h>
#include <stdint.h>

/* Safety cap to prevent malicious/buggy programs from exhausting kernel heap */
#define EXEC_MAX_ARGS    64
#define EXEC_MAX_ARG_LEN 255

#define ptr_valid(ptr) \
    ((uint32_t)(ptr) >= VMM_USER_START && (uint32_t)(ptr) <= VMM_USER_END)

/**
 * Copy a NULL-terminated array of userspace strings into kernel memory.
 * Returns the number of strings copied (written to *count_out) and a
 * freshly allocated kernel array in *kdst_out.  Both the pointer array and
 * each individual string are separately kmalloc'd so they can be freed with
 * exec_free_string_array().  Returns 0 on success, -1 on invalid pointer.
 * If usrc is NULL the function succeeds with an empty result.
 */
static int exec_copy_string_array(
    const char** usrc,
    const char*** kdst_out,
    int* count_out
) {
    if (usrc == NULL || !ptr_valid(usrc)) {
        *kdst_out = NULL;
        *count_out = 0;
        return 0;
    }

    /* Count valid entries up to the safety cap. */
    int count = 0;
    while (count < EXEC_MAX_ARGS) {
        if (!ptr_valid(&usrc[count])) return -1;
        if (usrc[count] == NULL) break;
        if (!ptr_valid(usrc[count])) return -1;
        count++;
    }

    const char** kdst = kmalloc(sizeof(char*) * (size_t)count);

    for (int i = 0; i < count; i++) {
        /* Bounded copy: scan up to EXEC_MAX_ARG_LEN bytes for the NUL. */
        const char* src = usrc[i];
        size_t len = 0;
        while (len < EXEC_MAX_ARG_LEN && src[len] != '\0') len++;

        char* copy = kmalloc(len + 1);
        memcpy(copy, src, len);
        copy[len] = '\0';
        kdst[i] = copy;
    }

    *kdst_out = kdst;
    *count_out = count;
    return 0;
}

static void exec_free_string_array(const char** arr, int count) {
    if (arr == NULL) return;
    for (int i = 0; i < count; i++) kfree((void*)arr[i]);
    kfree(arr);
}

int sys_exec(const char* path, const char** uargv, const char** uenvp)
{
    vfs_node_t* node = vfs_resolve(path);
    if (node == NULL) {
        return SYSCALL_ERROR;
    }
    if (!(node->type & VFS_TYPE_FILE)) {
        return SYSCALL_ERROR;
    }

    vfs_file_t* file = vfs_open(node, VFS_MODE_READONLY);
    if (file == NULL) {
        return SYSCALL_ERROR;
    }

    size_t size = file->node->length;
    void* buf = kmalloc(size);
    if (buf == NULL) {
        vfs_close(file);
        return SYSCALL_ERROR;
    }

    vfs_read(file, size, buf);
    vfs_close(file);

    /* Copy argv and envp from userspace into kernel memory before calling
     * task_exec(), which switches page directories and invalidates all
     * userspace pointers. */
    int argc = 0;
    const char** argv = NULL;
    int envc = 0;
    const char** envp = NULL;

    if (exec_copy_string_array(uargv, &argv, &argc) < 0) {
        kfree(buf);
        return SYSCALL_ERROR;
    }
    if (exec_copy_string_array(uenvp, &envp, &envc) < 0) {
        exec_free_string_array(argv, argc);
        kfree(buf);
        return SYSCALL_ERROR;
    }

    int ret = task_exec(buf, size, argc, argv, envc, envp);

    kfree(buf);
    exec_free_string_array(argv, argc);
    exec_free_string_array(envp, envc);
    return ret;
}
