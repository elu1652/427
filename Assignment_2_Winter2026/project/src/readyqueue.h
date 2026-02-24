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

void rq_prepend(PCB *p);

void rq_mt_init(void);
void rq_mt_shutdown(void);
PCB *rq_dequeue_blocking(void);  


#endif