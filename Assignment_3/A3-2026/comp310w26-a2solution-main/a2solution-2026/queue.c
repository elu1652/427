#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "pcb.h"
#include "queue.h"

struct queue {
    struct PCB *head;
};

struct queue *alloc_queue() {
    struct queue *q = malloc(sizeof(struct queue));
    q->head = NULL;
    return q;
}

void free_queue(struct queue *q) {

    struct PCB *p = q->head;
    while (p) {
        struct PCB *next = p->next;
        free_pcb(p);
        p = next;
    }
    free(q);
}

int program_already_scheduled(struct queue *q, char *name) {
    struct PCB *p = q->head;
    while (p) {
        if (strcmp(p->name, name) == 0) return 1;
        p = p->next;
    }
    return 0;
}


void enqueue_ignoring_priority(struct queue *q, struct PCB *pcb) {
    pcb->next = q->head;
    q->head = pcb;
}

void enqueue_fcfs(struct queue *q, struct PCB *pcb) {
    assert(pcb->next == NULL);
    struct PCB *p = q->head;

    if (!p) {
        q->head = pcb;
        return;
    }

    while (p->next) {
        p = p->next;
    }

    p->next = pcb;
}

void enqueue_sjf(struct queue *q, struct PCB *pcb) {
    size_t dur = pcb->duration;

    struct PCB *p = q->head;

    if (!p || p->duration > dur) {
        pcb->next = p;
        q->head = pcb;
        return;
    }

    while (p->next) {
        if (p->next->duration > dur) {
            pcb->next = p->next;
            p->next = pcb;
            return;
        }
        p = p->next;
    }

    p->next = pcb;
}

void enqueue_aging(struct queue *q, struct PCB *pcb) {

    if (q->head && q->head->duration == pcb->duration && pcb->pc) {
        enqueue_ignoring_priority(q, pcb);
    } else {
        enqueue_sjf(q, pcb);
    }
}


struct PCB *dequeue_typical(struct queue *q) {
    if (q->head == NULL) {
        return NULL;
    }

    // q -> head -> next
    struct PCB *head = q->head;
    // q -> next
    q->head = head->next;

    head->next = NULL;
    return head;
}


#ifdef NDEBUG
#define debug_with_age(q)
#else
#define debug_with_age(q) __debug_with_age(q)
#endif

void __debug_with_age(struct queue *q) {
    struct PCB *pcb = q->head;
    printf("q");
    while (pcb) {
        printf(" -> %ld %s", pcb->duration, pcb->name);
        pcb = pcb->next;
    }
    printf("\n");
}

struct PCB *dequeue_aging(struct queue *q) {
    debug_with_age(q);
    struct PCB *r = dequeue_typical(q);

    struct PCB *p = q->head;
    while (p) {
        if (p->duration > 0) {
            p->duration--;
        }
        p = p->next;
    }

    return r;
}

bool is_queue_empty(struct queue *q) {
    if (q == NULL) return true;
    return q->head == NULL;
}