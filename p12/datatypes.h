#include <ucontext.h>

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

// Estrutura que define uma tarefa
typedef struct task_t {
  struct task_t *prev;
  struct task_t *next;

  struct queue_t *queue;
  struct task_t *dependents_queue;

  int tid;
  int static_prio;
  int dynamic_prio;

  int activations;
  unsigned int creation_time;
  unsigned int last_activation_time;
  unsigned int execution_time;
  unsigned int processor_time;

  int exit_code;

  unsigned int waking_time;

  char task_type;
  char task_state;

    ucontext_t context;
} task_t;

// estrutura que define um semáforo
typedef struct
{
  struct task_t* queue;
  int value;
  unsigned char active;

} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  int hold_capacity;
  int tasks_retained;
  int active;

  struct task_t* queue;

} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct {
    int is_active;
    int message_size_limit;
    int stored_messages_number_limit;
    int stored_messages_number;
    
    void* data;

    semaphore_t buffer;
    semaphore_t item;
    semaphore_t spot;
} mqueue_t ;

#endif
