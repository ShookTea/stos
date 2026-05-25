#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

/**
 * If set to 1 - enable sending debug messages to the terminal
 */
#define KERNEL_DEBUG_TERMINAL 0

/**
 * If set to 1 - enable sending debug messages to serial port. Needs to be
 * compared with KERNEL_DEBUG_TERMINAL - when terminal debugging is enabled,
 * there's a separate check for "KERNEL_DEBUG_COM".
 */
#define KERNEL_DEBUG_COM 1

#define KERNEL_DEBUG_ANY (KERNEL_DEBUG_TERMINAL || KERNEL_DEBUG_COM)

typedef enum {
    DBC_ACPI,
    DBC_ATA,
    DBC_ATAPI,
    DBC_ELF,
    DBC_GRUB,
    DBC_IDT,
    DBC_MEM,
    DBC_OTHER,
    DBC_PAGING,
    DBC_PS2,
    DBC_RTC,
    DBC_SCHEDULER,
    DBC_SYSCALL,
    DBC_TASK,
    DBC_VFS,
    DBC_VFS_DEV,
    DBC_VFS_EXT2,
    DBC_VFS_ISO9660,
    DBC_VFS_PIPE,
    DBC_VGA,
} debug_channel_t;


#if (KERNEL_DEBUG_ANY)
    void debug_puts(const char* string);
    void debug_puts_c(const debug_channel_t cat, const char* string);
    void debug_puts_cc(
        const debug_channel_t cat,
        const char* subcat,
        const char* string
    );
    void debug_printf(const char* format, ...);
    void debug_printf_c(const debug_channel_t cat, const char* format, ...);
    void debug_printf_cc(
        const debug_channel_t cat,
        const char* subcat,
        const char* format,
        ...
    );
#else
    // No-op definitions
    #define debug_puts(...) ((void)0)
    #define debug_puts_c(...) ((void)0)
    #define debug_puts_cc(...) ((void)0)
    #define debug_printf(...) ((void)0)
    #define debug_printf_c(...) ((void)0)
    #define debug_printf_cc(...) ((void)0)
#endif

#endif
