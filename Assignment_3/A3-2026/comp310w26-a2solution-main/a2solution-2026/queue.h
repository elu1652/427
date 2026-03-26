#pragma once
#include <stdbool.h>
struct queue;

#include "schedule_policy.h"


struct queue *alloc_queue();
void free_queue(struct queue *q);


int program_already_scheduled(struct queue *q, char *name);


void enqueue_ignoring_priority(struct queue *q, struct PCB *pcb);


void enqueue_fcfs(struct queue *q, struct PCB *pcb);

void enqueue_sjf(struct queue *q, struct PCB *pcb);

void enqueue_aging(struct queue *q, struct PCB *pcb);


struct PCB *dequeue_typical(struct queue *q);

struct PCB *dequeue_aging(struct queue *q);

bool is_queue_empty(struct queue *q);