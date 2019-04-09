#include <stdio.h>
#include <stdlib.h>

#include "pingpong.h"

#define STACK_SIZE 30000
#define DEBUG

int task_identifier = 0;

task_t main_task;
task_t *current_task;

/*===================================================================================*/

void pingpong_init() {
    #ifdef DEBUG
        printf ("Booting operational system - Starting main task with TID: %d ", task_identifier);
    #endif

    main_task.tid = task_identifier;
    current_task = &main_task;

    setvbuf(stdout, 0, _IONBF, 0);
}

int task_create(task_t *task, void (*start_func)(void *), void *arg) {
    getcontext(&task->context);

    char *stack = malloc(STACK_SIZE);
    if (stack) {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACK_SIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    } else {
        return -1;
    }

    task_identifier++;
    task->tid = task_identifier;

    makecontext(&task->context, (void*)(*start_func), 1, arg);

    #ifdef DEBUG
        printf ("Successfully created new task with TID: %d ", task->tid);
    #endif

    return task->tid;
}

void task_exit (int exitCode) {
    task_switch(&main_task);

    #ifdef DEBUG
        printf ("Gracefully exiting current task with TID: %d ", current_task->tid);
    #endif
}

int task_switch (task_t *task) {
    task_t *task_to_be_swapped;

    task_to_be_swapped = current_task;
    current_task = task;

    #ifdef DEBUG
        printf ("Trying to swap context from task with TID: %d to task with TID with TID: %d ", task_to_be_swapped->tid, current_task->tid);
    #endif

    swapcontext(&task_to_be_swapped->context, &current_task->context);

    #ifdef DEBUG
        printf ("Successfully swapped contexts! ");
    #endif

    return 0;
}

int task_id () {
    return current_task->tid;
}
