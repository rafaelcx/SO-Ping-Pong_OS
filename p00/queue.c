#include "queue.h"
#include <stdbool.h>
#include <stdio.h>

//================================================================================
// Auxiliary functions
//================================================================================

bool queue_exists(queue_t **queue) {
    if (queue == NULL) {
        printf("ERROR: Queue does not exists!");
        return false;
    }
    return true;
}

bool element_exists(queue_t *elem) {
    if (elem == NULL) {
        printf("ERROR: The element provided does not exists!");
        return false;
    }
    return true;
}

bool element_does_not_belong_to_another_queue(queue_t *elem) {
    if (elem->next != NULL && elem->prev != NULL) {
        printf("ERROR: The element provided already belongs to another queue!");
        return false;
    }
    return true;
}

bool is_empty_queue(queue_t **queue) {
    return *queue == NULL;
}

bool is_single_element_queue(queue_t *elem) {
    return elem->next == elem && elem->prev == elem;
}

//================================================================================
// Default functions
//================================================================================

int queue_size(queue_t *queue) {
    int size = 0;
    queue_t *traversable;

    if (queue == NULL) {
        return size;
    }
    traversable = queue;
    do {
      size++;
      traversable = traversable->next;
    } while (traversable != queue);

    return size;
}

void queue_append(queue_t **queue, queue_t *elem) {
    if (queue_exists(queue)) {
        if (element_exists(elem)) {
            if (element_does_not_belong_to_another_queue(elem)) {
                if (is_empty_queue(queue)) {
                    elem->next = elem;
                    elem->prev = elem;
                    *queue = elem;
                } else {
                    elem->prev = (*queue)->prev;
                    elem->next = *queue;

                    (*queue)->prev->next = elem;
                    (*queue)->prev = elem;
                }
            }
        }
    }
}

queue_t *queue_remove (queue_t **queue, queue_t *elem) {
    if (queue_exists(queue)) {
        if (!is_empty_queue(queue)) {
            if (element_exists(elem)) {

                queue_t *aux = *queue;
                do {
                    if (aux == elem) {
                        if (is_single_element_queue(elem)) {
                            aux->next = NULL;
                            aux->prev = NULL;
                            *queue = NULL;
                        } else {
                            if (*queue == elem) {
                                *queue = (*queue)->next;
                            }
                            aux->prev->next = aux->next;
                            aux->next->prev = aux->prev;
                            aux->next = NULL;
                            aux->prev = NULL;
                        }

                        return aux;
                    }
                    aux = aux->next;
                } while (aux != *queue);
            }

        } else {
            printf("ERROR: Cannot perform this operation on empty queue!");
        }
    }
    return NULL;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*)) {
    queue_t *aux;

    printf("%s: [", name);
    if (queue != NULL) {
        aux = queue;
        do {
            print_elem(aux);
            aux = aux->next;

            if(aux != queue) {
                printf(" ");
            }

        } while(aux != queue);
    }
    printf("]\n");
}
