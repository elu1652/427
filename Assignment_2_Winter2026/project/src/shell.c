#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[]) {
    printf("Shell version 1.4 created December 2024\n\n");
    fflush(stdout);                    // Make sure prompt shows up immediately

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default

    int interactive = isatty(fileno(stdin)); // Check if shell is in interactive mode or batch mode

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }
    
    //init shell memory
    mem_init();
    while(1){
        if(interactive){ // Print prompt only in interactive mode and not batch mode
            printf("%c ", prompt);
        }
        if(fgets(userInput, MAX_USER_INPUT-1, stdin) == NULL){
            break; // Exit on EOF
        }
        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99);	// ignore all other errors
        memset(userInput, 0, sizeof(userInput));
    }
    return 0;
}

int wordEnding(char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ';
}

int parseInput(char inp[]) {
    int errorCode;
    char *saveptr;
    char *command = strtok_r(inp, ";", &saveptr); // Split by ';' to handle multiple commands
    int commandCount = 0; //Max of 10 commands per input line

    while (command != NULL && commandCount++ < 10) {
        // Process each command separately
        char tmp[200], *words[100];                            
        int ix = 0, w = 0;
        int wordlen;
        
        
        for (ix = 0; command[ix] == ' ' && ix < 1000; ix++); // skip white spaces
        if(command[ix] == '\0' || command[ix] == '\n') { //If it's an empty command
            command = strtok_r(NULL, ";", &saveptr); // Get next command from same input string
            continue;
        }
        while (command[ix] != '\n' && command[ix] != '\0' && ix < 1000) {
            // extract a word
            for (wordlen = 0; !wordEnding(command[ix]) && ix < 1000; ix++, wordlen++) {
                tmp[wordlen] = command[ix];                        
            }
            tmp[wordlen] = '\0';
            words[w] = strdup(tmp);
            w++;
            if (command[ix] == '\0') break;
            ix++; 
        }
        words[w] = NULL; // Null-terminate the array of words
        errorCode = interpreter(words, w);
        if (errorCode != 0) { // Break if there was an error
            return 1;
        }
        command = strtok_r(NULL, ";", &saveptr); // Get next command from same input string
    }

    
    return errorCode;
}
