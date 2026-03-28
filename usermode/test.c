#include <stdio.h>
#include <unistd.h>

char* data = "Hello, world!\n";

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
    print(data);
    print("Holy shit.\n");
    puts("Foo");

    return 0;
}
