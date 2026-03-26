#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#include <stdio.h>

#define MEM_SIZE 1000
#define CODE_MEM_SIZE 1000
#define MAX_CODE_LINE 100


void mem_init(void);
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);


int code_mem_load_script(FILE *fp, int *out_start, int *out_len);
char *code_mem_get_line(int index);
void code_mem_free_range(int start, int len);


#endif