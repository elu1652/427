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
    FILE *p = fopen(script, "rt");
    if (p == NULL) return badcommandFileDoesNotExist();
    int start, len;
    if (code_mem_load_script(p, &start, &len) != 0) {
        fclose(p);
        printf("Bad command: File too large\n"); 
        return 1;
    }
    fclose(p);
    PCB *proc = pcb_create(start, len);
    if (!proc) {
        code_mem_free_range(start, len);
        printf("Bad command: Out of memory\n");
        return 1;
    }
    rq_enqueue(proc);
    if (scheduler_mt_is_enabled()) return 0;
    scheduler_run_fcfs();
    return 0;
}
/*int source(char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");      // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput(line);     // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);

    return errCode;
}*/

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
        args_size--; // pretend MT isn't there for parsing policy/#/scripts
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

    // Determine if policy is valid
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

    // Check for duplicate script names
    for (int i = 1; i <= n_scripts; i++) {
        for (int j = i + 1; j <= n_scripts; j++) {
            if (strcmp(args[i], args[j]) == 0) {
                printf("bad command: exec\n");
                return 1;
            }
        }
    }

    PCB *pcbs[3] = {NULL, NULL, NULL};
    int starts[3] = {0, 0, 0};
    int lens[3]   = {0, 0, 0};

    for (int i = 0; i < n_scripts; i++) {
        FILE *fp = fopen(args[i+1], "r");
        if(!fp) {
            printf("Bad command: File not found\n");
            for (int k = 0; k < i; k++) {
                if (pcbs[k]) {
                    code_mem_free_range(starts[k], lens[k]);
                    free(pcbs[k]);
                }
            }
            return 1;
        }
        int rc = code_mem_load_script(fp, &starts[i], &lens[i]);
        fclose(fp);
        if (rc != 0) {
            printf("error: code memory full\n");
            for ( int k=0; k<i; k++) {
                if (pcbs[k]){
                    code_mem_free_range(starts[k], lens[k]);
                    free(pcbs[k]);
                }
            }
            return 1;
        }
        pcbs[i] = pcb_create(starts[i], lens[i]);
        if (!pcbs[i]) {
            code_mem_free_range(starts[i], lens[i]);
            for (int k = 0; k < i; k++) {
                if (pcbs[k]){
                    code_mem_free_range(starts[k], lens[k]);
                    free(pcbs[k]);
                }
            }
            printf("Bad command: exec\n");
            return 1;
        }
        // rq_enqueue(pcbs[i]);// FCFS
    }

    PCB *batch = NULL;
    int batch_start = 0;
    int batch_len = 0;

    if(batch_len > 0) {
        batch = pcb_create(batch_start, batch_len);
        if (!batch) {
            code_mem_free_range(batch_start, batch_len);
            printf("Bad command: exec\n");
            for (int k = 0; k < n_scripts; k++) {
                if (pcbs[k]){
                    code_mem_free_range(starts[k], lens[k]);
                    free(pcbs[k]);
                }
            }
            return 1;
        }
    }
    if (mt && !scheduler_mt_is_enabled()) {
        scheduler_mt_enable(policy);   // starts workers + rq_mt_init()
    }
    if(batch){
            rq_prepend(batch);
    }
    if (strcmp(policy, "AGING") == 0) {
        // enqueue in reverse so that "insert before equals" becomes FIFO among equals overall
        for (int i = n_scripts - 1; i >= 0; i--) {
            rq_enqueue_aging(pcbs[i]);
        }
    } else {
        for (int i = 0; i < n_scripts; i++) {
            rq_enqueue(pcbs[i]);
        }
    }

    if (background) return 0;    

    if (mt || scheduler_mt_is_enabled()) {
        return 0;
    }
    if (scheduler_active && !background) {
        return 0;
    }

    if (strcmp(policy, "FCFS") == 0) scheduler_run_fcfs();
    else if ( strcmp(policy, "SJF") == 0) scheduler_run_sjf();
    else if (strcmp(policy, "RR")== 0) scheduler_run_rr();
    else if (strcmp(policy, "AGING") == 0) scheduler_run_aging();
    else if (strcmp(policy, "RR30") == 0) scheduler_run_rr30();
    else {printf("Bad command: exec \n"); return 1;}
    return 0;
}
