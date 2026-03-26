#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "shellmemory.h"


#define true 1
#define false 0


// Helper functions
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i])
            matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else
        return 0;
}



// for exec memory

struct program_line {
    int allocated; // for sanity-checking
    char *line;
};

struct program_line linememory[MEM_SIZE];
size_t next_free_line = 0;

void reset_linememory_allocator() {
    next_free_line = 0;
    assert_linememory_is_empty();
}

void assert_linememory_is_empty() {
    for (size_t i = 0; i < MEM_SIZE; ++i) {
        assert(!linememory[i].allocated);
        assert(linememory[i].line == NULL);
    }
}

void init_linemem() {
    for (size_t i = 0; i < MEM_SIZE; ++i) {
        linememory[i].allocated = false;
        linememory[i].line = NULL;
    }
}

size_t allocate_line(const char *line) {
    if (next_free_line >= MEM_SIZE) {
        // out of memory!
        return (size_t)(-1);
    }
    size_t index = next_free_line++;
    assert(!linememory[index].allocated);

    linememory[index].allocated = true;
    linememory[index].line = strdup(line);
    return index;
}

void free_line(size_t index) {
    free(linememory[index].line);
    linememory[index].allocated = false;
    linememory[index].line = NULL;
}

const char *get_line(size_t index) {
    assert(linememory[index].allocated);
    return linememory[index].line;
}


// Shell memory functions

struct memory_struct { // block or line
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];



void mem_init() {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    return;
}

//get value based on input key
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            return strdup(shellmemory[i].value);
        }
    }
    return NULL;
}
