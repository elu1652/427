#include <stdlib.h>
#include "pcb.h"
#include "readyqueue.h"
#include <pthread.h>

static PCB *head = NULL;
static PCB *tail = NULL;

static int mt_enabled = 0;
static int mt_shutdown = 0;
static int avail = 0;

static pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  notempty = PTHREAD_COND_INITIALIZER;

void rq_init(void) {
    head = tail = NULL;
}
int rq_is_empty(void) {
    if (!mt_enabled) return head == NULL;
    pthread_mutex_lock(&qlock);
    int empty = (avail <= 0);   // or (head == NULL)
    pthread_mutex_unlock(&qlock);
    return empty;
}
void rq_enqueue(PCB *p) {
    p -> next = NULL;
    if (!mt_enabled) {
        if (!tail) head = tail = p;
        else { tail->next = p; tail = p; }
        return;
    }
    pthread_mutex_lock(&qlock);
    if (!tail){
        head = tail = p;
    } else {
        tail-> next = p;
        tail = p;
    }
    avail++;
    pthread_cond_signal(&notempty);
    pthread_mutex_unlock(&qlock);
}
void rq_enqueue_aging(PCB *p) {
    p-> next = NULL;
    if( head == NULL) {
        head = tail = p;
        return;
    }
    if (p -> job_score < head -> job_score) {
        p-> next = head;
        head = p;
        return;
    }
    PCB *prev = head;
    PCB *cur = head -> next;
    while (cur != NULL && cur -> job_score <= p-> job_score) {
        prev = cur;
        cur = cur -> next;
    }
    prev -> next = p;
    p-> next = cur;
    if ( cur == NULL ) {
        tail = p;
    }
}
void rq_age_all_except(PCB *running) {
    PCB *old = head;
    head = tail = NULL;
    while (old != NULL) {
        PCB *next = old -> next;
        old -> next = NULL;
        if (old != running) {
            if (old -> job_score > 0) old -> job_score--;
        }
        rq_enqueue_aging(old);
        old = next;
    }
}
PCB *rq_dequeue(void){
    if (!head) {
        return NULL;
    }
    PCB *p = head;
    head = head-> next;
    if (!head) {
        tail = NULL;
    }
    p-> next = NULL;
    return p;
}

PCB *rq_dequeue_sjf(void){
    if (!head) {
        return NULL;
    }
    PCB *prev_min = NULL;
    PCB *min_node = head;
    PCB *prev = NULL;
    PCB *cur = head;
    while (cur) {
        if ( cur -> length < min_node -> length) {
            min_node = cur;
            prev_min = prev;
        }
        prev = cur;
        cur = cur -> next;
    }
    if (prev_min == NULL){
        head = min_node -> next;
    } else {
        prev_min -> next = min_node -> next;
    }
    if(min_node == tail) {
        tail = prev_min;
    }
    min_node -> next = NULL;
    return min_node;
}

void rq_prepend(PCB *p) {
    if (!p) return;

    if (!mt_enabled) {
        p->next = head;
        head = p;
        if (tail == NULL) tail = p;
        return;
    }

    pthread_mutex_lock(&qlock);

    p->next = head;
    head = p;
    if (tail == NULL) tail = p;
    avail++;
    pthread_cond_signal(&notempty);
    pthread_mutex_unlock(&qlock);
}

void rq_mt_init(void) {
    mt_enabled = 1;
    mt_shutdown = 0;
    avail = 0;
}

void rq_mt_shutdown(void) {
    pthread_mutex_lock(&qlock);
    mt_shutdown = 1;
    pthread_cond_broadcast(&notempty);
    pthread_mutex_unlock(&qlock);
}

PCB *rq_dequeue_blocking(void) {
    pthread_mutex_lock(&qlock);
    while (avail <= 0 && !mt_shutdown) {
        pthread_cond_wait(&notempty, &qlock);
    }
    if (mt_shutdown) {
        pthread_mutex_unlock(&qlock);
        return NULL;
    }
    // normal dequeue, but under lock
    PCB *p = head;
    head = head->next;
    if (!head) tail = NULL;
    p->next = NULL;
    avail--;
    pthread_mutex_unlock(&qlock);
    return p;
}