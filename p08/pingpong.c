#include <stdio.h>
#include <stdlib.h>

#include "pingpong.h"
#include "queue.h"
#include <signal.h>
#include <sys/time.h>

#define STACK_SIZE 30000

#define TYPE_SYSTEM_TASK 's'
#define TYPE_KERNEL_TASK 'k'

#define TASK_STATUS_EXECUTING 'e'
#define TASK_STATUS_READY 'r'
#define TASK_STATUS_SUSPENDED 's'
#define TASK_STATUS_EXITED 'x'

#define QUANTUM_SIZE_IN_TICKS 20

#define DEBUG_P06

int task_identifier = 0;
int remaining_ticks = QUANTUM_SIZE_IN_TICKS;

unsigned int system_time;

task_t main_task;
task_t *current_task;
task_t dispatcher_task;

queue_t *ready_queue;

struct sigaction action;
struct itimerval timer;
struct itimerval stop_timer = {0};

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

void printSettingPriorityOutsideOfAcceptableBoundsMessage() {
    #ifdef DEBUG
        printf("Trying to set task priority outside of acceptable UNIX bounds, setting to default instead.");
    #endif
}

void printDefinedTaskTypeMessage(task_t* task) {
    #ifdef DEBUG
        printf("Task with TID: %d type has been defined to: %c \n", task->tid, task->task_type);
    #endif
}

void printEndOfQuantumMessage() {
    #ifdef DEBUG
        printf("End of task with TID: %d quantum \n", current_task->tid);
    #endif
}

void printTaskTimeCosumingStatistics(task_t* task) {
    #ifdef DEBUG_P06
        printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations \n", task->tid, task->execution_time, task->processor_time, task->activations);
    #endif
}

void printUnsuccessfullJoinMessage() {
    #ifdef DEBUG
        printf("Error while trying to execute join operation, task gived does not exists\n");
    #endif
}

void printSuccessfullJoinMessage(int task_tid) {
    #ifdef DEBUG
        printf("Successfully joined current task with task -> %d \n", task_tid);
    #endif
}

//================================================================================
// Auxiliary functions
//================================================================================

int isPriorityBetweenAcceptableBounds(int prio) {
    if (prio >= -20 && prio <= 20) {
        return 1;
    }
    printSettingPriorityOutsideOfAcceptableBoundsMessage();
    return 0;
}

void defineTaskType(task_t* task) {
    if (task == &dispatcher_task) {
        task->task_type = TYPE_KERNEL_TASK;
    } else {
        task->task_type = TYPE_SYSTEM_TASK;
    }
    printDefinedTaskTypeMessage(task);
}

//===================================================================================
// Preemption related functions
//===================================================================================

void resetRemainingTicksQuantity() {
    remaining_ticks = QUANTUM_SIZE_IN_TICKS;
}

void systemClockTickHandler() {
    system_time++; // Incremeting system clock

    if(current_task != &dispatcher_task) {
        remaining_ticks--;

        if(remaining_ticks <= 0) {
            printEndOfQuantumMessage();
            resetRemainingTicksQuantity();
            task_yield();
        }
    }
}

//===================================================================================
// Scheduler function
//===================================================================================

task_t* scheduler() {
    queue_t* ready_queue_iterator;
    task_t* next_task = (task_t*)ready_queue;

    if (ready_queue != NULL) {

        // Priority policy
        ready_queue_iterator = ready_queue;
        do {
            task_t* task = (task_t*)ready_queue_iterator; // Type cast to be able to manipulate element as a task

            if (task->dynamic_prio < next_task->dynamic_prio) {
                next_task = task;
            }

            // Removing the custom tie breaker between tasks with same dynamic priority

            // if (task->dynamic_prio == next_task->dynamic_prio && task->tid != next_task->tid) {
            //     printf("uguas\n");
            //     if (task->tid < next_task->tid) {
            //         next_task = task;
            //     }
            // }

            ready_queue_iterator = ready_queue_iterator->next;
        } while (ready_queue_iterator != ready_queue);

        //Task aging
        ready_queue_iterator = ready_queue;
        do {
            task_t* task = (task_t*)ready_queue_iterator; // Type cast to be able to manipulate element as a task
            if (task->tid != next_task->tid) {
                task->dynamic_prio--;
            }
            ready_queue_iterator = ready_queue_iterator->next;
        } while (ready_queue_iterator != ready_queue);

        next_task->dynamic_prio = next_task->static_prio;

        return next_task;
    }

    return NULL;
}

//===================================================================================
// Dispatcher function
//===================================================================================

void dispacherBody(void *arg) {
    while (queue_size(ready_queue) > 0) {

        task_t* next_task = scheduler();

        if (next_task) {
            next_task->queue = NULL;
            next_task->task_state = TASK_STATUS_EXECUTING;
            queue_remove(&ready_queue, (queue_t*)next_task);
            task_switch(next_task);
        }
    }
    task_exit(0);
}

//===================================================================================
// Timer functions
//===================================================================================

void registerTimerSignal() {
    action.sa_handler = systemClockTickHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if(sigaction(SIGALRM, &action, 0) < 0) {
        perror("Sigaction error: ");
        exit(1);
    }
}

void setTimerValues() {
    timer.it_value.tv_usec = 1000;
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec = 0;
}

void setTimer() {
    if(setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Setitimer error: ");
        exit(1);
    }
}

void buildTimer() {
    registerTimerSignal();
    setTimerValues();
    setTimer();
}

//===================================================================================
// Project functions
//===================================================================================

void pingpong_init() {
    setvbuf(stdout, 0, _IONBF, 0);
    printBootMessage();

    // Creating main task
    main_task.tid = task_identifier;
    main_task.creation_time = systime();
    main_task.activations = 0;
    main_task.execution_time = 0;
    main_task.processor_time = 0;
    main_task.task_type = TYPE_SYSTEM_TASK;
    main_task.queue = (queue_t*)&ready_queue;
    main_task.dependents_queue = NULL;
    current_task = &main_task;

    // Initializing system clock
    system_time = 0;
    buildTimer();

    // Creating dispatcher task
    task_create(&dispatcher_task, &dispacherBody, NULL);

    // Preemption related stuff
    buildTimer();

    task_yield();
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
        task->queue = ready_queue;
        task->task_state = TASK_STATUS_READY;
    }

    task->static_prio = 0;
    task->dynamic_prio = 0;

    task->creation_time = systime();
    task->activations = 0;
    task->execution_time = 0;
    task->processor_time = 0;
    task->dependents_queue = NULL;
    defineTaskType(task);

    printTaskCreatedMessage(task->tid);

    return task->tid;
}

void task_exit (int exitCode) {
    printTaskExitingMessage(current_task->tid);

    current_task->task_state = TASK_STATUS_EXITED;
    current_task->exit_code = exitCode;

    // Resuming all tasks that were joined with this current exiting task
    while(current_task->dependents_queue) {
        task_resume(current_task->dependents_queue);
    }

    current_task->execution_time = systime() - current_task->creation_time;
    printTaskTimeCosumingStatistics(current_task);

    if (current_task->tid == 1) {
        task_switch(&main_task);
    } else {
        task_switch(&dispatcher_task);
    }
}

int task_switch (task_t *task) {
    task_t* task_to_be_swapped = current_task;
    current_task = task;

    printTryingToSwapContextsMessage(task_to_be_swapped->tid, current_task->tid);

    task->activations++;
    task->last_activation_time = systime();
    task_to_be_swapped->processor_time += systime() - task_to_be_swapped->last_activation_time;
    
    swapcontext(&task_to_be_swapped->context, &current_task->context);

    printContextSwappedMessage();

    return 0;
}

void task_yield() {
    if (current_task->task_state != TASK_STATUS_SUSPENDED) {
        queue_append((queue_t**)&ready_queue, (queue_t*)current_task);
        current_task->queue = ready_queue;
        current_task->task_state = TASK_STATUS_READY;
    }

    task_switch(&dispatcher_task);
}

void task_suspend(task_t *task, task_t **queue) {
    if (task == NULL) {
        task = current_task;
    }

    if (queue != NULL) {
        if (task->queue != NULL) {
            queue_remove((queue_t**)(task->queue), (queue_t*)task);
        }
        queue_append((queue_t**)queue, (queue_t*)task);
        task->queue = (queue_t*)queue;
    }

    task->task_state = TASK_STATUS_SUSPENDED;
}

void task_resume(task_t *task) {
    if (task->queue != NULL) {
        queue_remove((queue_t**)(task->queue), (queue_t*)task);
    }

    queue_append(&ready_queue, (queue_t*)task);
    task->queue = ready_queue;
    task->task_state = TASK_STATUS_READY;
}

int task_join(task_t* task) {
    if (task == NULL || task->task_state == TASK_STATUS_EXITED) {
        printUnsuccessfullJoinMessage();
        return -1;
    }

    setitimer(ITIMER_REAL, &stop_timer, NULL);
    task_suspend(NULL, &(task->dependents_queue));
    setitimer(ITIMER_REAL, &timer, NULL);

    printSuccessfullJoinMessage(task->tid);

    task_yield();
    return(task->exit_code);
}

void task_setprio(task_t *task, int prio) {
    if (isPriorityBetweenAcceptableBounds(prio) == 1) {
        if (task != NULL) {
            task->static_prio = prio;
            task->dynamic_prio = prio;
        }
        current_task->static_prio = prio;
        current_task->dynamic_prio = prio;
    } else {
        task->static_prio = 0;
        task->dynamic_prio = 0;
    }
}

int task_getprio(task_t *task) {
    if (task != NULL) {
            return task->static_prio;
    }
    return current_task->static_prio;
}

int task_id () {
    return current_task->tid;
}

unsigned int systime () {
    return system_time;
}
