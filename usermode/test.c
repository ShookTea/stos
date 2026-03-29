#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    puts("Hello from userspace!");
    void* array1 = malloc(sizeof(int) * 16);
    void* array2 = malloc(sizeof(int) * 32);
    void* array3 = malloc(sizeof(int) * 16);

    printf("Addr array1 = %#x\n", array1);
    printf("Addr array2 = %#x\n", array2);
    printf("Addr array3 = %#x\n", array3);

    ((int*)array1)[5] = 32;

    printf("array1 val = %d\n", ((int*)array1)[5]);

    return 0;
}
