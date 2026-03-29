#include <stdio.h>
#include <unistd.h>

int main(void)
{
    puts("Hello from userspace!");
    void* heap_end = brk(0);
    printf("Current heap end: %#x\n", heap_end);
    heap_end = brk(heap_end + 10000);
    printf("Heap end after increment: %#x\n", heap_end);
    heap_end = brk(0);
    printf("Current heap end: %#x\n", heap_end);
    heap_end = brk(heap_end - 5000);
    printf("Heap end after decrement: %#x\n", heap_end);
    heap_end = brk(0);
    printf("Current heap end: %#x\n", heap_end);

    return 0;
}
