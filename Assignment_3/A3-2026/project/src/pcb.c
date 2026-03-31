#include <stdlib.h>
#include "pcb.h"

static int next_pid = 1;

PCB *pcb_create(int length, int pages_max) {
    PCB *p = malloc(sizeof(PCB));
    if(!p) return NULL;
    p->pid = next_pid++;
    // p->start = start;
    p->length = length;
    p->pages_max = pages_max;
    p->page_table = NULL;
    p->script = NULL;

    p->pc=0;
    p->next = NULL;
    p->job_score = length;
    return p;
}
int pcb_is_done(PCB *p) {
    return p -> pc >= p -> length;
}
/*
int pcb_next_abs_index(PCB *p) {
    return p -> start + p -> pc;
}
*/


int pcb_get_page(PCB *p) {
    return p->pc / 3;
}

int pcb_get_offset(PCB *p) {
    return p->pc % 3;
}

int pcb_next_abs_index(PCB *p) {
    int page = pcb_get_page(p);
    int offset = pcb_get_offset(p);

    if (page < 0 || page >= p->pages_max) return -1;

    int frame = p->page_table[page];
    if (frame < 0) return -1;

    return frame * 3 + offset;
}

void pcb_free(PCB *p) {
    if(p) {
        //free(p->page_table);
        free(p);
    }
}