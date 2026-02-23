#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

static char *codeMem[CODE_MEM_SIZE];
static int codeTop = 0; //next free slot
int code_mem_load_script(FILE *fp, int *out_start, int *out_len) {
    char buf[MAX_CODE_LINE + 5];
    int start = codeTop;
    int count = 0;
    while (fgets(buf , sizeof(buf), fp) != NULL){
        if ( codeTop >= CODE_MEM_SIZE) return -1;
        codeMem[codeTop]=strdup(buf);
        if (!codeMem[codeTop]) return -1;
        codeTop++;
        count++;
    }
    *out_start = start;
    *out_len = count;
    return 0;
}
char *code_mem_get_line(int index) {
    if(index < 0 || index >= CODE_MEM_SIZE) return NULL;
    return codeMem[index];
}
void code_mem_free_range(int start, int len) {
    for (int i=0; i< len; i++) {
        int idx = start + i;
        if(idx >= 0 && idx < CODE_MEM_SIZE && codeMem[idx]){
            free(codeMem[idx]);
            codeMem[idx]=NULL;
        }
    }
}
struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

// Helper functions
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i]) matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else return 0;
}

// Shell memory functions

void mem_init(){
    int i;
    for (i = 0; i < MEM_SIZE; i++){		
        shellmemory[i].var   = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, "none") == 0){
            shellmemory[i].var   = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    return;
}

//get value based on input key
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            return strdup(shellmemory[i].value);
        } 
    }
    return NULL;
}