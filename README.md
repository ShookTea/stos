# StOS

### Prerequisites

1. GCC Cross-Compiler - you can find installation instructions in the 
  ([OsDev Wiki](https://wiki.osdev.org/GCC_Cross-Compiler))
    - used commands for building are: `i686-elf-gcc`, `i686-elf-as`
2. Grub - for building bootable ISO files, without having to write a bootloader
    - used commands are: `grub-file`, `grub-mkrescue`
3. Qemu virtual machine - for launching the OS somewhere
    - used command is: `qemu-system-i386`

### Basic development tools

Running just `make all` should build a bootable `build/stos.iso` file.
Running `make qemu` will both create the bootable ISO file and immediately
launch the Qemu virtual machine with that ISO plugged into a CD-ROM.

To improve how CLang handles the code (errors, "Go to definition" for libraries)
you can install `bear` package and then run `bear -- make qemu`. It will create
a `compile_commands.json` file (which is added to `.gitignore`) containing
arguments for compiler that is respected by CLang.

All text printed to VGA text mode is by default sent to COM1 port, which Qemu
then prints to the command line.

### Source code structure
```
<root directory>
└── src/                    # Main source directory 
    ├── arch/               # Files specific to a specific infrastructure
    │   └── i386/
    ├── include/kernel/     # Common header files used by the kernel that are
    │                       # implemented in the infra-specific code
    ├── kernel/             # Main source code of the kernel
    ├── libc/include/       # implementation of libc standard library
    └── grub.cfg            # Bootloader configuration
```

### What's implemented

- Booting
- GDT, interrupts, IRQ
- Paging
- Physical memory allocation (buddy allocator)
- PIT driver
- PS/2 driver
- PS/2 keyboard driver
- Debugging CLI - running tests, displaying stats, shutdown, reboot
- kmalloc/kfree
- Unit tests for libc
- memory leak test - check memory stats, then run all mem tests multiple times,
  then check memory stats again and compare if there are any changes
- Basic ACPI parsing - shutdown & reboot support
- Virtual File System (with initrd)

### Short-term todo list

- ANSI escape codes support in `terminal.c`
- Support for more than 1 GiB of RAM
  - that's currently not working because we're trying to map all physical
    RAM in paging, which doesn't make sense above 1 GiB. We should implement
    demand paging instead.
- Thread safety for memory allocation code
