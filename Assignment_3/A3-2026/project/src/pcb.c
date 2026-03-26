#include <stdlib.h>
#include "pcb.h"

static int next_pid = 1;

PCB *pcb_create(int start, int length) {
    PCB *p = malloc(sizeof(PCB));
    if(!p) return NULL;
    p->pid = next_pid++;
    p->start = start;
    p->length = length;
    p->pc=0;
    p->next = NULL;
    p->job_score = length;
    return p;
}
int pcb_is_done(PCB *p) {
    return p -> pc >= p -> length;
}
int pcb_next_abs_index(PCB *p) {
    return p -> start + p -> pc;
}