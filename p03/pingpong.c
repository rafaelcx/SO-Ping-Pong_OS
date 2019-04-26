#include <stdio.h>
#include <stdlib.h>

#include "pingpong.h"
#include "queue.h"

#define STACK_SIZE 30000

int task_identifier = 0;

task_t main_task;
task_t *current_task;
task_t dispatcher_task;

queue_t *ready_queue;
queue_t *suspended_queue;

//================================================================================
// Debug functions
//================================================================================

void printBootMessage() {
    #ifdef DEBUG
        printf ("Booting operational system - Starting main task with TID: %d ", task_identifier);
    #endif
}

void printTaskCreatedMessage(int task_tid) {
    #ifdef DEBUG
        printf ("Successfully created new task with TID: %d ", task_tid);
    #endif
}

void printTaskExitingMessage(int current_task_tid) {
    #ifdef DEBUG
        printf ("Gracefully exiting current task with TID: %d ", current_task_tid);
    #endif
}

void printTryingToSwapContextsMessage(int task_to_be_swapped_tid, int current_task_tid) {
    #ifdef DEBUG
        printf ("Trying to swap context from task with TID: %d to task with TID with TID: %d ", task_to_be_swapped_tid, current_task_tid);
    #endif
}

void printContextSwappedMessage() {
    #ifdef DEBUG
        printf ("Successfully swapped contexts! ");
    #endif
}


//================================================================================
// Auxiliary functions
//================================================================================

task_t* scheduler() {
    // Implemented using FCFS style
    return (task_t*)ready_queue;
}

void dispacherBody(void *arg) {
    while (queue_size(ready_queue) > 0) {

        task_t* next_task = scheduler();

        if (next_task) {
            queue_remove(&ready_queue, (queue_t*)next_task);
            task_switch(next_task);
        }
    }
    task_exit(0);
}


/*===================================================================================*/

void pingpong_init() {
    printBootMessage();

    main_task.tid = task_identifier;
    current_task = &main_task;

    task_create(&dispatcher_task, &dispacherBody, NULL);

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

    // Append on ready_queue all tasks that are not main_task or dispatcher_task
    if (task->tid > 1) {
        queue_append(&ready_queue, (queue_t*)task);
        task->queue = (queue_t*)&ready_queue;
    }

    printTaskCreatedMessage(task->tid);

    return task->tid;
}

void task_exit (int exitCode) {
    if (current_task->tid == 1) {
        task_switch(&main_task);
    } else {
        task_switch(&dispatcher_task);
    }

    //Free alocated memory from task

    printTaskExitingMessage(current_task->tid);
}

int task_switch (task_t *task) {
    task_t *task_to_be_swapped;

    task_to_be_swapped = current_task;
    current_task = task;

    printTryingToSwapContextsMessage(task_to_be_swapped->tid, current_task->tid);

    swapcontext(&task_to_be_swapped->context, &current_task->context);

    printContextSwappedMessage();

    return 0;
}

void task_yield() {
    if (current_task->tid != 0) {
        queue_append(&ready_queue, (queue_t*)current_task);
    }
    task_switch(&dispatcher_task);
}

void task_suspend(task_t *task, task_t **queue) {
    if (queue != NULL) {
        queue_t* removed_element = queue_remove((queue_t**)&queue, (queue_t*)task);
        queue_append((queue_t**)&queue, removed_element);
        task->queue = (queue_t*)&queue;
    } else {
        task = current_task;
    }
}

void task_resume(task_t *task) {
    if (task->queue != NULL) {
        queue_remove(&task->queue, (queue_t*)task);
    }
    queue_append(&ready_queue, (queue_t*)task);
    task->queue = ready_queue;
}

int task_id () {
    return current_task->tid;
}
