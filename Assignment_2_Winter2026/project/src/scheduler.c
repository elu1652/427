#include <string.h>
#include <stdlib.h>
#include "scheduler.h"
#include "readyqueue.h"
#include "shellmemory.h"

int parseInput(char ui[]);

#define MAX_USER_INPUT 1000  
void scheduler_run_fcfs(void) {
    while (!rq_is_empty()) {
        PCB *p = rq_dequeue();
        while (!pcb_is_done(p)) {
            int absIdx = pcb_next_abs_index(p);
            char *line = code_mem_get_line(absIdx);
            if (!line) { // safety
                p->pc++;
                continue;
            }
            char temp[MAX_USER_INPUT];
            strncpy(temp, line, MAX_USER_INPUT - 1);
            temp[MAX_USER_INPUT - 1] = '\0';
            parseInput(temp);
            p->pc++;
        }
        //cleanup 
        code_mem_free_range(p->start, p->length);
        free(p);
    }
}
void scheduler_run_sjf(void) {
    while (!rq_is_empty()){
        PCB *p = rq_dequeue_sjf();
        while (!pcb_is_done(p)) {
            int absIdx = pcb_next_abs_index(p);
            char *line = code_mem_get_line(absIdx);
            if (line) {
                char temp[MAX_USER_INPUT];
                strncpy(temp, line, MAX_USER_INPUT-1);
                temp[MAX_USER_INPUT-1] = '\0';
                parseInput(temp);
            }
            p -> pc++;
        }
        code_mem_free_range( p-> start, p-> length);
        free(p);
    }
}

#define RR_QUANTUM 2
void scheduler_run_rr(void){
    while (!rq_is_empty()){
        PCB *p = rq_dequeue();

        int steps =0;
        while (steps < RR_QUANTUM && !pcb_is_done(p)) {
            int absIdx = pcb_next_abs_index(p);
            char *line = code_mem_get_line(absIdx);

            if(line) {
                char temp[MAX_USER_INPUT];
                strncpy(temp, line, MAX_USER_INPUT -1);
                temp[MAX_USER_INPUT-1] = '\0';
                parseInput(temp);
            }
            p -> pc++;
            steps++;
        }
        if(pcb_is_done(p)) {
            code_mem_free_range( p-> start, p-> length);
            free(p);
        } else {
            rq_enqueue(p);
        }
    }
}

void scheduler_run_aging(void) {
    while(!rq_is_empty()) {
        PCB *p = rq_dequeue();
        if (!pcb_is_done(p)){
            int absIdx = pcb_next_abs_index(p);
            char *line = code_mem_get_line(absIdx);
            if(line) {
                char temp[MAX_USER_INPUT];
                strncpy(temp, line, MAX_USER_INPUT -1);
                temp[MAX_USER_INPUT -1] = '\0';
                parseInput(temp);
            }
            p -> pc++;
        }
        if (pcb_is_done(p)){
            code_mem_free_range( p-> start, p-> length);
            free(p);
        } else {
            rq_age_all_except(p);
            rq_enqueue_aging(p);
        }
    }
}