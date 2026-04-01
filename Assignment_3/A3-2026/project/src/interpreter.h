#ifndef INTERPRETER_H
#define INTERPRETER_H

int interpreter(char *command_args[], int args_size);
int help();

typedef struct ScriptEntry {
    char *name;
    int length;
    int pages_max;
    int *page_table;
    int ref_count;
    struct ScriptEntry *next;
} ScriptEntry;

void update_frame_time(int frame);
void cleanup_all_scripts(void);

#endif