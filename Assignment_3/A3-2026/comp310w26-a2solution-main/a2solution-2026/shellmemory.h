#include <stdio.h>
#define MEM_SIZE 1000

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);


void assert_linememory_is_empty(void);
size_t allocate_line(const char *line);
void free_line(size_t index);
const char *get_line(size_t index);
void reset_linememory_allocator(void);