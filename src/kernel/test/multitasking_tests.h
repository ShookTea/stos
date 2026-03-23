#ifndef KERNEL_MULTITASKING_TESTS_H
#define KERNEL_MULTITASKING_TESTS_H

#include <stdio.h>
#include "../task/task.h"
#include "../task/scheduler.h"


static volatile int counter1 = 0;
static volatile int counter2 = 0;

static void test_task_1()
{
    while (1) {
        counter1++;
        printf("\033[91mTask 1: counter = %d\033[m\n", counter1);
        // Busy wait to let time pass
        for (volatile int i = 0; i < 10000000; i++);
    }
}

static void test_task_2()
{
    while (1) {
        counter2++;
        printf("\033[94mTask 2: counter = %d\033[m\n", counter2);
        // Busy wait to let time pass
        for (volatile int i = 0; i < 10000000; i++);
    }
}

void task_run_basic_test(void) {
    printf("\n=== Multitasking Test ===\n");
    printf("Creating two test tasks...\n");

    task_t* task1 = task_create("test1", test_task_1, true);
    task_t* task2 = task_create("test2", test_task_2, true);

    printf("Adding tasks to scheduler...\n");
    scheduler_add_task(task1);
    scheduler_add_task(task2);

    printf("Tasks created! They should start running automatically.\n");
    printf("You should see alternating output from Task 1 and Task 2.\n");

    // Let them run forever (they're in the scheduler now)
}
#endif
