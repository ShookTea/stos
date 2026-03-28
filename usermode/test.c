#include <stdio.h>
#include <sys/syscall.h>

char* data = "Hello, world!";

static int write(int fd, const char* buf, unsigned int count)
{
    return syscall(SYS_WRITE, fd, (int)buf, count);
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
    puts("Foo");

    return 0;
}
