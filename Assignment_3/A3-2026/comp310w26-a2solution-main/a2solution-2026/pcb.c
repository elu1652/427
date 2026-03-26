#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset
#include "shell.h" // MAX_USER_INPUT
#include "shellmemory.h"
#include "pcb.h"

int pcb_has_next_instruction(struct PCB *pcb) {
    return pcb->pc < pcb->line_count;
}

size_t pcb_next_instruction(struct PCB *pcb) {
    size_t i = pcb->line_base + pcb->pc;
    pcb->pc++;
    return i;
}

struct PCB *create_process(const char *filename) {
    FILE *script = fopen(filename, "rt");
    if (!script) {
        perror("failed to open file for create_process");
        return NULL;
    }
    struct PCB *pcb = create_process_from_FILE(script);
    pcb->name = strdup(filename);
    return pcb;
}


struct PCB *create_process_from_FILE(FILE *script) {
    struct PCB *pcb = malloc(sizeof(struct PCB));
    static pid fresh_pid = 1;
    pcb->pid = fresh_pid++;
    pcb->name = "";
    pcb->next = NULL;
    pcb->pc = 0;
    pcb->line_count = 0;
    pcb->line_base = 0;
    char linebuf[MAX_USER_INPUT];
    while (!feof(script)) {
        memset(linebuf, 0, sizeof(linebuf));
        fgets(linebuf, MAX_USER_INPUT, script);

        size_t index = allocate_line(linebuf);
        if (index == (size_t)(-1)) {
            free_pcb(pcb);
            fclose(script);
            return NULL;
        }

        if (pcb->line_count == 0) {
            pcb->line_base = index;
        }
        pcb->line_count++;
    }
    fclose(script);
    pcb->duration = pcb->line_count;
    return pcb;
}

void free_pcb(struct PCB *pcb) {
    for (size_t ix = pcb->line_base; ix < pcb->line_base + pcb->line_count; ++ix) {
        free_line(ix);
    }
    if (strcmp("", pcb->name)) {
        free(pcb->name);
    }
    free(pcb);
}
