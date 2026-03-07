#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/tty.h>

#if !defined(__i386__)
#error "This kernel needs to be compiled with a ix86-elf compiler"
#endif


void kernel_main(void)
{
    tty_initialize();
    tty_writestring("Hello, kernel World!");
    tty_enable_cursor(0, 15);
    tty_update_cursor(21, 0);
}
