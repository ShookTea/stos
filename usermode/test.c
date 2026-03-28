char* data = "Hello, world!";

static int write(int fd, const char* buf, unsigned int count)
{
    int ret;
    __asm__ volatile(
        "int $0x80\n"
        : "=a"(ret) // output: EAX = return value
        : "a"(4),   // input: EAX = SYS_WRITE (4)
          "c"(fd),  // input: ECX = fd (arg1)
          "d"(buf), // input: EDX = buf (arg2)
          "b"(count)// input: EBX = count (arg3)
        : "memory"  // clobber: memory (syscall may access user memory)
    );
    return ret;
}

// Helper to print a null-terminated string
static void print(const char* str)
{
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    write(1, str, len);
}

int main(void)
{
    print("Hello from userspace!\n");
    print("This is a test program.\n");
    print("It works!\n");
    print("Holy shit.\n");

    return 0;
}
