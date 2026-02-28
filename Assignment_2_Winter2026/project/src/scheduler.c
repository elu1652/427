#include <string.h>
#include <stdlib.h>
#include "scheduler.h"
#include "readyqueue.h"
#include "shellmemory.h"
#include <pthread.h>

int parseInput(char ui[]);
int scheduler_active = 0;

static int mt_enabled = 0;
static int mt_quantum = 0;     // 2 for RR, 30 for RR30
static int mt_started = 0;

static pthread_t w1, w2;

static pthread_mutex_t exec_lock = PTHREAD_MUTEX_INITIALIZER;

#define MAX_USER_INPUT 1000  

// First Come First Served scheduling
void scheduler_run_fcfs(void) {
    scheduler_active = 1;
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
        // Clean up
        code_mem_free_range(p->start, p->length);
        free(p);
    }
    scheduler_active = 0;
}

// Shortest Job First scheduling
void scheduler_run_sjf(void) {
    scheduler_active = 1;
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
    scheduler_active = 0;
}

// Round Robin with quantum of 2
#define RR_QUANTUM 2
void scheduler_run_rr(void){
    scheduler_active = 1;
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
    scheduler_active = 0;
}

// Aging-based priority scheduling (preempts when higher priority job arrives)
void scheduler_run_aging(void) {
    scheduler_active = 1;

    PCB *cur = NULL;

    while (cur != NULL || !rq_is_empty()) {

        // If we don't have a current running job, take the best from the queue
        if (cur == NULL) {
            cur = rq_dequeue();
            if (cur == NULL) break;
        }

        // Run exactly 1 instruction of cur
        if (!pcb_is_done(cur)) {
            int absIdx = pcb_next_abs_index(cur);
            char *line = code_mem_get_line(absIdx);
            if (line) {
                char temp[MAX_USER_INPUT];
                strncpy(temp, line, MAX_USER_INPUT - 1);
                temp[MAX_USER_INPUT - 1] = '\0';
                parseInput(temp);
            }
            cur->pc++;
        }

        // If finished, cleanup and fetch next
        if (pcb_is_done(cur)) {
            code_mem_free_range(cur->start, cur->length);
            free(cur);
            cur = NULL;
            continue;
        }

        // Age everyone else 
        rq_age_all_except(NULL);

        // preempt only if someone now has a smaller job_score than cur
        PCB *head = rq_peek_head();     
        if (head != NULL && head->job_score < cur->job_score) {
            rq_enqueue_aging(cur);
            cur = NULL;  // next iteration will dequeue the best
        }
    }

    scheduler_active = 0;
}

// Round Robin with quantum of 30
#define RR30_QUANTUM 30
void scheduler_run_rr30(void){
    scheduler_active = 1;
    while (!rq_is_empty()){
        PCB *p = rq_dequeue();

        int steps =0;
        while (steps < RR30_QUANTUM && !pcb_is_done(p)) {
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
    scheduler_active = 0;
}

// Worker thread function for multithreading
static void *worker_main(void *arg) {
    (void)arg;
    for (;;) {
        PCB *p = rq_dequeue_blocking();
        if (p == NULL) {
            // shutdown was requested
            break;
        }

        int steps = 0;
        while (steps < mt_quantum && !pcb_is_done(p)) {
            int absIdx = pcb_next_abs_index(p);
            char *line = code_mem_get_line(absIdx);
            if (line) {
                char temp[MAX_USER_INPUT];
                strncpy(temp, line, MAX_USER_INPUT - 1);
                temp[MAX_USER_INPUT - 1] = '\0';

                pthread_mutex_lock(&exec_lock);
                parseInput(temp);
                pthread_mutex_unlock(&exec_lock);
            }
            p->pc++;
            steps++;
        }

        if (pcb_is_done(p)) {
            // cleanup
            code_mem_free_range(p->start, p->length);
            free(p);
        } else {
            rq_enqueue(p);
        }
    }
    return NULL;
}

int scheduler_mt_is_enabled(void) { return mt_enabled; }

// Enable multithreading with worker threads
void scheduler_mt_enable(const char *policy) {
    if (mt_enabled) return;

    // Only tested with RR/RR30
    if (strcmp(policy, "RR30") == 0) mt_quantum = 30;
    else mt_quantum = 2;

    rq_mt_init();

    mt_enabled = 1;
    scheduler_active = 1;

    if (!mt_started) {
        mt_started = 1;
        pthread_create(&w1, NULL, worker_main, NULL);
        pthread_create(&w2, NULL, worker_main, NULL);
    }
}

// Shutdown multithreading by signaling workers and joining threads
void scheduler_mt_shutdown(void) {
    if (!mt_enabled) return;

    rq_mt_shutdown();

    // join threads
    pthread_join(w1, NULL);
    pthread_join(w2, NULL);

    mt_enabled = 0;
    scheduler_active = 0;
}