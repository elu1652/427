#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"


static char *frameStore[FRAME_STORE_SIZE];
static int frameUsed[FRAME_COUNT];

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

void code_mem_init(void) {
    for (int i = 0; i < FRAME_STORE_SIZE; i++) {
        frameStore[i] = NULL;
    }
    for (int f = 0; f < FRAME_COUNT; f++) {
        frameUsed[f] = 0;
    }
}

int code_mem_find_free_frame(void) {
    for (int f = 0; f < FRAME_COUNT; f++) {
        if (frameUsed[f] == 0) return f;
    }
    return -1;
}

int code_mem_load_page(FILE *fp) {
    int frame = code_mem_find_free_frame();
    if (frame == -1) return -1; // No free frame available

    int base = frame * FRAME_SIZE;
    char buf[MAX_CODE_LINE + 5]; 

    for (int i = 0; i < FRAME_SIZE; i++) {
        if (fgets(buf, sizeof(buf), fp) != NULL) {
            frameStore[base + i] = strdup(buf); // Load line into frame
            if (!frameStore[base + i]) return -1; 
        } else {
            frameStore[base + i] = NULL; 
        }
    }

    frameUsed[frame] = 1;
    return frame;
}

char *code_mem_get_line(int index) {
    if (index < 0 || index >= FRAME_STORE_SIZE) return NULL;
    return frameStore[index];
}

void code_mem_free_frame(int frame) {
    if (frame < 0 || frame >= FRAME_COUNT) return;

    int base = frame * FRAME_SIZE;
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (frameStore[base + i]) {
            free(frameStore[base + i]);
            frameStore[base + i] = NULL;
        }
    }
    frameUsed[frame] = 0;
    
}

int code_mem_load_page_into_frame(FILE *fp, int frame) {
    int base = frame * FRAME_SIZE;
    char buf[MAX_CODE_LINE + 5];

    for (int i = 0; i < FRAME_SIZE; i++) {
        if (fgets(buf, sizeof(buf), fp) != NULL) {
            frameStore[base + i] = strdup(buf);
        } else {
            frameStore[base + i] = NULL;
        }
    }

    frameUsed[frame] = 1;
    return frame;
}