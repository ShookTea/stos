#include "./tty.h"
#include "kernel/terminal.h"

size_t tty_write(
    vfs_file_t* file __attribute__((unused)),
    size_t offset __attribute__((unused)),
    size_t size,
    const void* ptr
) {
    for (size_t i = 0; i < size; i++) {
        char c = ((char*)ptr)[i];
        terminal_write_char(c);
    }

    return size;
}
