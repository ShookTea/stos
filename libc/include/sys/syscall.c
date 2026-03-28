#if !(defined(__is_libk))

int syscall(int number, int arg1, int arg2, int arg3)
{
    int ret;
    __asm__ volatile(
        "int $0x80\n"
        : "=a"(ret)
        : "a"(number),
          "c"(arg1),
          "d"(arg2),
          "b"(arg3)
        : "memory"
    );
    return ret;
}

#endif
