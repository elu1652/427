#ifndef PCB_H
#define PCB_H

typedef struct PCB {
    int pid;
    int start;   
    int length;    
    int pc;    
    int job_score;    
    struct PCB *next;
} PCB;

PCB *pcb_create(int start, int length);
int pcb_is_done(PCB *p);
int pcb_next_abs_index(PCB *p);

#endif
