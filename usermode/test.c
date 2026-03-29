#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    puts("Hello from userspace!");
    void* array1 = malloc(sizeof(int) * 1024);
    void* array2 = malloc(sizeof(int) * 1024);
    void* array3 = malloc(sizeof(int) * 1024);

    printf("Addr array1 = %#x\n", array1);
    printf("Addr array2 = %#x\n", array2);
    printf("Addr array3 = %#x\n", array3);

    ((int*)array1)[5] = 32;
    ((int*)array2)[5] = 64;
    ((int*)array3)[5] = 128;

    printf("array1 val = %d\n", ((int*)array1)[5]);
    printf("array2 val = %d\n", ((int*)array2)[5]);
    printf("array3 val = %d\n", ((int*)array3)[5]);

    return 0;
}
