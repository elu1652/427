#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#include <stdio.h>

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 12
#endif

#ifndef VAR_MEM_SIZE
#define VAR_MEM_SIZE 20
#endif

#define MAX_CODE_LINE 100

#define FRAME_SIZE 3
#define FRAME_COUNT (FRAME_STORE_SIZE / FRAME_SIZE)
#define MEM_SIZE VAR_MEM_SIZE


void mem_init(void);
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);

void code_mem_init(void);
int code_mem_find_free_frame(void);
int code_mem_load_page(FILE *fp);
char *code_mem_get_line(int index);
void code_mem_free_frame(int frame);
int code_mem_load_page_into_frame(FILE *fp, int frame);


#endif