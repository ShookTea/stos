#include <stddef.h>
#include "../syscall.h"
#include <kernel/memory/vmm.h>
#include <kernel/terminal.h>

int sys_write(int fd, const void* buf, size_t count)
{
    // For now only support stdout (fd=1)
    if (fd != 1) {
        return -1;
    }

    // Validate user pointer is in user space
    if ((uint32_t)buf < VMM_USER_START || (uint32_t)buf >= VMM_USER_END) {
        return -1;
    }

    // Write to terminal
    const char* str = (const char*)buf;
    for (size_t i = 0; i < count; i++) {
        terminal_write_char(str[i]);
    }

    return (int)count;
}
