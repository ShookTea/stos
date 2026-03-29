#include <stdio.h>
#include <unistd.h>

int main(void)
{
    puts("Hello from userspace!");
    void* heap_end = sbrk(0);
    printf("Current heap end: %#x\n", heap_end);
    heap_end = sbrk(10000);
    printf("Heap end after increment: %#x\n", heap_end);
    heap_end = sbrk(0);
    printf("Current heap end: %#x\n", heap_end);
    heap_end = sbrk(-5000);
    printf("Heap end after decrement: %#x\n", heap_end);
    heap_end = sbrk(0);
    printf("Current heap end: %#x\n", heap_end);

    return 0;
}
