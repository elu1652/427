#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "pcb.h"
#include "readyqueue.h"
#include "scheduler.h"
#include "shell.h"
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include "interpreter.h"


// modify source() for A2 part1

int MAX_ARGS_SIZE = 6;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);
int echo(char *string);
int badcommandFileDoesNotExist();
int my_ls();
int my_mkdir(char *dirName);
int my_touch(char *fileName);
int my_cd(char *dirname);
int run(char **args);
int exec(char *args[], int args_size);

static int load_page_with_eviction(FILE *fp, ScriptEntry *entry, int page, int *did_evict);
static int find_lru_frame(void);

// Data structure to represent a loaded script and its associated info
typedef struct frameInfo{
    ScriptEntry *owner;
    int page_number;
    int valid;
    int last_used;
} frameInfo;

static ScriptEntry *script_list = NULL;
static frameInfo frame_info[FRAME_COUNT];
static int lru_clock = 0; // Global clock for LRU tracking

// Helper function to find a script entry by filename
static ScriptEntry *find_script(const char *filename) {
    ScriptEntry *cur = script_list;
    while (cur != NULL) {
        if (strcmp(cur->name, filename) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

// Helper function to count lines in a script file (used to determine length and pages needed)
static int count_script_lines(FILE *fp) {
    char buf[MAX_CODE_LINE + 5];
    int count = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        count++;
    }

    rewind(fp);
    return count;
}

// Free all pages associated with a PCB 
static void free_loaded_pages(PCB *p) {
    if (!p) return;

    for (int i = 0; i < p->pages_max; i++) {
        if (p->page_table[i] != -1) {
            int frame = p->page_table[i];
            code_mem_free_frame(frame);
            frame_info[frame].owner = NULL;
            frame_info[frame].page_number = -1;
            frame_info[frame].valid = 0;
            p->page_table[i] = -1;
            frame_info[frame].last_used = -1;
        }
    }
}

// Load the initial pages of a script into memory (called when first loading a script)
static int load_script_pages(FILE *fp, PCB *p) {
    // Load the first pages, maximum of 2 pages
    int initial_pages = (p->pages_max < 2) ? p->pages_max : 2;

    for (int page = 0; page < initial_pages; page++) {
        // Attempt to load page, evicting if necessary
        if (load_page_with_eviction(fp, p->script, page,NULL) < 0) {
            return -1;
        }
    }

    return 0;
}

// Load a page while evicting if no frames available
static int load_page_with_eviction(FILE *fp, ScriptEntry *entry, int page, int *did_evict) {
    int frame = code_mem_load_page(fp);
    // If we got a frame, just use it
    if (frame >= 0) {
        // Mark that no eviction occurred
        if (did_evict) *did_evict = 0;
        // Update page table and frame info
        entry->page_table[page] = frame;
        frame_info[frame].owner = entry;
        frame_info[frame].page_number = page;
        frame_info[frame].valid = 1;
        frame_info[frame].last_used = lru_clock++;
        return frame;
    }

    // No free frame, use LRU to find a victim
    int victim = find_lru_frame();
    if (victim == -1) {
        return -1; // No frame available and no victim found 
    }
    
    // Mark that eviction occurred
    if (did_evict) *did_evict = 1;

    printf("Page fault! Victim page contents:\n\n");
    int base = victim * FRAME_SIZE; // Calculate the base line number of the victim page
    for (int i = 0; i < FRAME_SIZE; i++) {
        char *line = code_mem_get_line(base + i); // Get the line from code memory
        if (line) printf("%s", line); // Print the line if it exists
    }
    printf("\nEnd of victim page contents.\n");

    // Evict the victim page
    ScriptEntry *victim_owner = frame_info[victim].owner; // Get the owner of the victim page
    int victim_page = frame_info[victim].page_number; // Get the page number of the victim page in its owner's page table

    // Invalidate the victim page in its owner's page table
    if (victim_owner && victim_page != -1) {
        victim_owner->page_table[victim_page] = -1;
    }
    
    // Free the victim frame
    code_mem_free_frame(victim); 

    frame_info[victim].owner = NULL;
    frame_info[victim].page_number = -1;
    frame_info[victim].valid = 0;
    frame_info[victim].last_used = -1;

    // Load the new page into the now free victim frame
    frame = code_mem_load_page_into_frame(fp, victim);
    if (frame < 0) return -1;

    entry->page_table[page] = frame;
    frame_info[frame].owner = entry;
    frame_info[frame].page_number = page;
    frame_info[frame].valid = 1;
    frame_info[frame].last_used = lru_clock++;
    return frame;
}

// Load a script as a process, creating a PCB and loading initial pages
static PCB *load_script_as_process(const char *filename) {
    ScriptEntry *entry = find_script(filename);

    if(entry != NULL) {
        entry->ref_count++; // Increment reference count for shared script entry
        PCB *p = pcb_create(entry->length, entry->pages_max);
        if (!p) {
            entry->ref_count--; // Decrement ref count if PCB creation fails
            return NULL;
        }
        p->script = entry;
        p->page_table = entry->page_table;

        return p;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    int length = count_script_lines(fp);
    int pages_max = (length + FRAME_SIZE - 1) / FRAME_SIZE; // Calculate pages needed, rounding up

    PCB *p = pcb_create(length, pages_max);
    if (!p) {
        fclose(fp);
        return NULL;
    }

    // Create a shared script entry
    entry = malloc(sizeof(ScriptEntry));
    if (!entry) {
        pcb_free(p);
        fclose(fp);
        return NULL;
    }

    // Name the entry
    entry->name = strdup(filename);
    if(!entry->name) {
        free(entry);
        pcb_free(p);
        fclose(fp);
        return NULL;
    }

    // Initialize the rest of the entry
    entry->length = length;
    entry->pages_max = pages_max;
    entry->page_table = malloc(pages_max * sizeof(int));
    if (!entry->page_table) {
        free(entry->name);
        free(entry);
        pcb_free(p);
        fclose(fp);
        return NULL;
    }

    for (int i = 0; i < pages_max; i++) {
        entry->page_table[i] = -1; // Initialize page table entries to -1 (indicating not allocated)
    }
    
    p->script = entry;
    p->page_table = entry->page_table;

    // Load initial pages of the script into memory
    if (load_script_pages(fp, p) != 0) {
        free(entry->page_table);
        free(entry->name);
        free(entry);
        pcb_free(p);
        fclose(fp);
        return NULL;    
    }

    // Add the new entry to the global script list
    entry->ref_count = 1;
    entry->next = script_list;
    script_list = entry;


    fclose(fp);
    return p;
}

// Helper function to release pages of a PCB
void release_script_pages(PCB *p) {
    if (!p) return;

    ScriptEntry *cur = p->script;
    if (!cur) {
        pcb_free(p);
        return;
    }
    
    cur->ref_count--;
    
    pcb_free(p);
}

// Cleanup function to free all loaded scripts and their associated frames (called on shutdown)
void cleanup_all_scripts(void) {
    ScriptEntry *cur = script_list;

    while (cur) {
        ScriptEntry *next = cur->next;

        for (int i = 0; i < cur->pages_max; i++) {
            if (cur->page_table[i] != -1) {
                int frame = cur->page_table[i];
                code_mem_free_frame(frame);
                frame_info[frame].owner = NULL;
                frame_info[frame].page_number = -1;
                frame_info[frame].valid = 0;
                frame_info[frame].last_used = -1;
                cur->page_table[i] = -1;
            }
        }

        free(cur->page_table);
        free(cur->name);
        free(cur);

        cur = next;
    }

    script_list = NULL;
}

// Helper function to load a missing page for a PCB (used during page fault handling)
int load_missing_page(PCB *p, int page, int *did_evict) {
    // Find the script entry for this PCB
    ScriptEntry *entry = p->script;
    if (!entry) return -1;

    FILE *fp = fopen(entry->name, "r");
    if (!fp) return -1;

    // Seek to the correct position in the file for the page we want to load
    char buf[MAX_CODE_LINE + 5];
    int lines_to_skip = page * FRAME_SIZE;
    for (int i = 0; i < lines_to_skip; i++) {
        if (fgets(buf, sizeof(buf), fp) == NULL) {
            fclose(fp);
            return -1;
        }
    }

    // Load the page with eviction if necessary
    int frame = load_page_with_eviction(fp, entry, page, did_evict);
    fclose(fp);
    return (frame < 0) ? -1 : 0;
}
void init_frame_info() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        frame_info[i].owner = NULL;
        frame_info[i].page_number = -1;
        frame_info[i].valid = 0;
        frame_info[i].last_used = -1;
    }
}

// Update the last used time for a frame (called on each access)
void update_frame_time(int frame) {
    if (frame < 0 || frame >= FRAME_COUNT) return;
    frame_info[frame].last_used = lru_clock++;
}

static int find_lru_frame(void) {
    int victim = -1;
    int oldest_time = 0;

    // Find the valid frame with the oldest last used time
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (frame_info[i].valid) {
            if (victim == -1 || frame_info[i].last_used < oldest_time) {
                victim = i;
                oldest_time = frame_info[i].last_used;
            }
        }
    }

    return victim;
}

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);
    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1)
            return badcommand();
        return my_ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2)
            return badcommand();
        return run(command_args);

    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3 || args_size > 6) 
            return badcommand();
        return exec(command_args, args_size);
    } // added this for A2
    else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    scheduler_mt_shutdown();
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    char *value = mem_get_value(var);
    if (value == NULL) {
        printf("Variable does not exist\n");
    } else {
        printf("%s\n", value);
    }
    return 0;
}

int source(char *script) {
    PCB *proc = load_script_as_process(script);
    if (!proc) {
        return badcommandFileDoesNotExist();
    }

    rq_enqueue(proc);

    if (scheduler_mt_is_enabled()) return 0;
    scheduler_run_fcfs();
    return 0;
}

int echo(char *string) {
    if (string[0] == '$') {
        // Print variable value
        char *varName = string + 1; // Skip the '$' character
        char *value = mem_get_value(varName);
        if (value != NULL) {
            printf("%s\n", value);
        } else {
            printf("\n"); // Print empty line if variable not found
        }
    } else {
        // Print the input
        printf("%s\n", string);
    }
    return 0;
}

int my_ls() {
    DIR *dir = opendir("."); // Open current directory
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    size_t cap = 64; // Initial capacity
    size_t n = 0;
    char **entries = malloc(cap * sizeof(char*)); // Allocate memory for entries
    if (entries == NULL) {
        perror("malloc");
        closedir(dir);
        return 1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) { // Read each entry
        if(n >= cap) { // Resize if necessary
            cap *= 2;
            char **temp = realloc(entries, cap * sizeof(char*));
            if (temp == NULL) {
                perror("realloc");
                for (size_t j = 0; j < n; j++) {
                    free(entries[j]);
                }
                free(entries);
                closedir(dir);
                return 1;
            }
            entries = temp;
        }
        entries[n] = strdup(entry->d_name); // Duplicate entry name
        if (entries[n] == NULL) {
            perror("strdup");
            for (size_t j = 0; j < n; j++) {
                free(entries[j]);   
            }
            free(entries);
        }
        n++;
    }
    closedir(dir); // Close directory

    int cmpfunc(const void *a, const void *b){
    
        return strcmp(*(const char **)a, *(const char **)b);
    }

    qsort(entries, n, sizeof(char*), cmpfunc); // Sort entries
    
    for (size_t i = 0; i < n; i++) { // Print and free entries
        printf("%s\n", entries[i]);
        free(entries[i]);
    }
    free(entries); // Free the entries array
    return 0;
}

int is_alphnum(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0; // Empty string is not alphanumeric
    }
    while (*str) {
        if (!isalnum((unsigned char)*str)) {
            return 0; // Found a non-alphanumeric character
        }
        str++;
    }
    return 1; // All characters are alphanumeric
}

int my_mkdir(char *dirName) {
    if(dirName[0] == '$'){
        char *varName = dirName + 1; // Skip the '$' character
        char *value = mem_get_value(varName);
        if (value != NULL) {
            if(is_alphnum(value)){
                dirName = value;
            } else {
                printf("Bad command: my_mkdir\n");
                return 1;   
            }
        } else {
            printf("Bad command: my_mkdir\n");
            return 1;   
        }
    }
    else{
        if(!is_alphnum(dirName)){
            printf("Bad command: my_mkdir\n");
            return 1;   
        }
    }
    if (mkdir(dirName, 0777) != 0) {
        printf("Bad command: my_mkdir\n");
        return 1;
    }
    
    return 0;
}

int my_touch(char *filename) {
    if(is_alphnum(filename) == 0){
        printf("Bad command: my_touch\n");
        return 1;   
    }
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        printf("Bad command: my_touch\n");
        return 1;
    }
    fclose(file);
    return 0;
}

int my_cd(char *dirname){
    if(is_alphnum(dirname) == 0){
        printf("Bad command: my_cd\n");
        return 1;   
    }
    if(chdir(dirname) != 0){
        printf("Bad command: my_cd\n");
        return 1;
    }
    return 0;
}

int run(char **args){
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return 1;
    } 

    if(pid == 0) {
        // Child process
        execvp(args[1], &args[1]);
        perror("execvp failed");
        exit(1);
    } 
    int status;
    waitpid(pid, &status, 0); // Wait for child process to finish
    return 0;

}


int exec(char *args[], int args_size) {
    int background = 0;
    char *policy = args[args_size - 1];
    int n_scripts = 0;
    int mt = 0;
    if (args_size >= 2 && strcmp(args[args_size - 1], "MT") == 0) {
        mt = 1;
        args_size--; // Exclude "MT" from further processing
    }
    if (args_size >= 2 && strcmp(args[args_size - 1], "#") == 0) { // Check if last argument is "#"
        background = 1;
        policy = args[args_size - 2];
        n_scripts = args_size - 3;   // exec + scripts + policy + #
    } else {
        background = 0;
        policy = args[args_size - 1];
        n_scripts = args_size - 2;   // exec + scripts + policy
    }

    // MT only supports RR and RR30
    if(mt){
        if (!(strcmp(policy, "RR") == 0 || strcmp(policy, "RR30") == 0)) {
            printf("bad command: exec\n");
            return 1;
        }
    }

    // Validate scripts count
    if (n_scripts < 1 || n_scripts > 3) {
        printf("bad command: exec\n");
        return 1;
    }

    // Validate scheduling policy
    int policy_ok =
        (strcmp(policy, "FCFS") == 0) ||
        (strcmp(policy, "SJF") == 0)  ||
        (strcmp(policy, "RR") == 0)   ||
        (strcmp(policy, "AGING") == 0)||
        (strcmp(policy, "RR30") == 0);
    if (!policy_ok) {
        printf("bad command: exec\n");
        return 1;
    }



    // Load each script file and create PCBs
    PCB *pcbs[3] = {NULL, NULL, NULL};

    for (int i = 0; i < n_scripts; i++) {
        pcbs[i] = load_script_as_process(args[i + 1]);
        if (!pcbs[i]) {
            printf("Bad command: File not found\n");
            for (int k = 0; k < i; k++) {
                if (pcbs[k]) {
                    release_script_pages(pcbs[k]);
                }
            }
            return 1;
        }
    }
    
    // Enable multithreading if requested
    if (mt && !scheduler_mt_is_enabled()) {
        scheduler_mt_enable(policy);   // starts workers + rq_mt_init()
    }
    
    // Enqueue processes to ready queue based on policy
    if (strcmp(policy, "AGING") == 0) {
        // enqueue in reverse if aging to ensure first script has highest priority
        for (int i = n_scripts - 1; i >= 0; i--) {
            rq_enqueue_aging(pcbs[i]);
        }
    } else {
        for (int i = 0; i < n_scripts; i++) {
            rq_enqueue(pcbs[i]);
        }
    }

    if (background) return 0;    

    // Return immediately if multithreading (scheduler handles)
    if (mt || scheduler_mt_is_enabled()) {
        return 0;
    }
    if (scheduler_active && !background) {
        return 0;
    }

    // Run scheduler with appropriate algorithm
    if (strcmp(policy, "FCFS") == 0) scheduler_run_fcfs();
    else if ( strcmp(policy, "SJF") == 0) scheduler_run_sjf();
    else if (strcmp(policy, "RR")== 0) scheduler_run_rr();
    else if (strcmp(policy, "AGING") == 0) scheduler_run_aging();
    else if (strcmp(policy, "RR30") == 0) scheduler_run_rr30();
    else {printf("Bad command: exec \n"); return 1;}
    return 0;
}
