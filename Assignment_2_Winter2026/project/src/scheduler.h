#ifndef SCHEDULER_H
#define SCHEDULER_H

extern int scheduler_active;

void scheduler_run_fcfs(void);
void scheduler_run_sjf(void);
void scheduler_run_rr(void);
void scheduler_run_aging(void);
void scheduler_run_rr30(void);
#endif
