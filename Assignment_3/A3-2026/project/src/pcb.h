#ifndef PCB_H
#define PCB_H

#include "interpreter.h"

typedef struct PCB {
    int pid;
    //int start;   Not needed when sharing memory
    int length;    
    int pc;    
    int job_score;    
    struct PCB *next;
    int pages_max;
    int *page_table;
    ScriptEntry *script;
} PCB;

PCB *pcb_create(int start, int length);
int pcb_is_done(PCB *p);
int pcb_next_abs_index(PCB *p);

void pcb_free(PCB *p);
int pcb_get_page(PCB *p);

#endif
