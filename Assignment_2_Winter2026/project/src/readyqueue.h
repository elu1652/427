#ifndef READYQUEUE_H
#define READYQUEUE_H

#include "pcb.h"

void rq_init(void);
int rq_is_empty(void);

void rq_enqueue(PCB *p);
PCB *rq_dequeue(void);

void re_enqueue_sjf(PCB *p);
PCB *rq_dequeue_sjf(void);

void rq_enqueue_aging(PCB *P);
void rq_age_all_except(PCB *running);


#endif