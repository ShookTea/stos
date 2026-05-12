#include "kernel/debug.h"
#include "kernel/serial.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#if (KERNEL_DEBUG_ANY)

static int cat_mask = (1 << DBC_ATA) | (1 << DBC_VFS_DEV);

static inline bool cat_suppressed(const debug_channel_t cat)
{
    return (cat_mask & (1 << cat)) != 0;
}

static inline int cat_color(const debug_channel_t cat)
{
    return (cat % 8) + 30;
}

static inline char* cat_label(const debug_channel_t cat)
{
    switch (cat) {
        case DBC_ACPI: return "ACPI";
        case DBC_ATA: return "ATA";
        case DBC_ATAPI: return "ATAPI";
        case DBC_ELF: return "ELF";
        case DBC_GRUB: return "GRUB";
        case DBC_IDT: return "IDT";
        case DBC_MEM: return "MEM";
        case DBC_OTHER: return ".";
        case DBC_PAGING: return "paging";
        case DBC_PS2: return "PS-2";
        case DBC_SCHEDULER: return "scheduler";
        case DBC_SYSCALL: return "syscall";
        case DBC_TASK: return "task";
        case DBC_VFS: return "VFS";
        case DBC_VFS_DEV: return "VFS/dev";
        case DBC_VFS_EXT2: return "VFS/ext2";
        case DBC_VFS_ISO9660: return "VFS/iso9660";
        case DBC_VGA: return "VGA";
    }
    return "";
}

void debug_puts(const char* string)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        for (;*string;string++) serial_put_c(*string);
        serial_put_c('\n');
    #elif KERNEL_DEBUG_TERMINAL
        puts(string);
    #endif
}

void debug_puts_c(const debug_channel_t cat, const char* string)
{
    if (cat_suppressed(cat)) return;
    debug_printf(
        "\033[%d;1m[%s]\033[0m %s\n",
        cat_color(cat),
        cat_label(cat),
        string
    );
}

void debug_puts_cc(
    const debug_channel_t cat,
    const char* subcat,
    const char* string
) {
    if (cat_suppressed(cat)) return;
    debug_printf(
        "\033[%d;1m[%s/%s]\033[0m %s\n",
        cat_color(cat),
        cat_label(cat),
        subcat,
        string
    );
}

void debug_printf(const char* format, ...)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        char buffer[200];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 200, format, args);
        va_end(args);
        char* string = buffer;
        for (;*string;string++) serial_put_c(*string);
    #elif KERNEL_DEBUG_TERMINAL
        va_list args;
        va_start(args, format);
        int res = vprintf(format, args);
        va_end(args);
    #endif
}

void debug_printf_c(const debug_channel_t cat_id, const char* format, ...)
{
    if (cat_suppressed(cat_id)) return;
    char* cat = cat_label(cat_id);
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        char buffer[200];
        snprintf(buffer, 200, "\033[%d;1m[%s]\033[0m ", cat_color(cat_id), cat);
        char* string = buffer;
        for (;*string;string++) serial_put_c(*string);
        memset(buffer, 0, 200);

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 200, format, args);
        va_end(args);
        string = buffer;
        for (;*string;string++) serial_put_c(*string);
    #elif KERNEL_DEBUG_TERMINAL
        printf("\033[%d;1m[%s]\033[0m ", cat_color(cat_id), cat);
        va_list args;
        va_start(args, format);
        int res = vprintf(format, args);
        va_end(args);
    #endif
}

void debug_printf_cc(
    const debug_channel_t cat_id,
    const char* subcat,
    const char* format,
    ...
) {
    if (cat_suppressed(cat_id)) return;
    char* cat = cat_label(cat_id);
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        char buffer[200];
        snprintf(
            buffer,
            200,
            "\033[%d;1m[%s/%s]\033[0m ",
            cat_color(cat_id),
            cat,
            subcat
        );
        char* string = buffer;
        for (;*string;string++) serial_put_c(*string);
        memset(buffer, 0, 200);

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 200, format, args);
        va_end(args);
        string = buffer;
        for (;*string;string++) serial_put_c(*string);
    #elif KERNEL_DEBUG_TERMINAL
        printf("\033[%d;1m[%s/%s]\033[0m ", cat_color(cat_id), cat, subcat);
        va_list args;
        va_start(args, format);
        int res = vprintf(format, args);
        va_end(args);
    #endif
}

#endif
