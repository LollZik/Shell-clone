#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"

int cmd_echo(InputBuffer *input) {
    input->valid_input = true;
    
    printf("%s\n", input->input + 5); 
    
    return 0;
}

int cmd_exit(InputBuffer *input) {
    exit(0);
    return 0; 
}

int cmd_type(InputBuffer *input){
    input->valid_input = true;
    bool found = false;
    char* command = strtok(NULL, " "); // get the second word from input
    if (command == NULL) return 0;

    for(int i = 0; i < num_commands; i++){
      if(strcmp(command,dispatch_table[i].name)==0){
        printf("%s is a shell builtin\n",command);
        found = true;
        break;
      }
    }
    if(!found){
      found = searchPATH(command);
      if(!found){
        printf("%s: not found\n",command);
      }
    }
    return 0;
}

Command dispatch_table[] = {
    {"echo", cmd_echo},
    {"exit", cmd_exit},
    {"type", cmd_type}
};

const int num_commands = sizeof(dispatch_table) / sizeof(Command);

bool checkInput(InputBuffer *inputBuffer){
    char *command = strtok(inputBuffer->input, " "); 
    if(command == NULL) { return false; }

    for(int i = 0; i < num_commands; i++){
        if(strcmp(command, dispatch_table[i].name) == 0){
            inputBuffer->valid_input = true;
            dispatch_table[i].func(inputBuffer);
            return true;
        }
    }
    inputBuffer->valid_input = false;
    printf("%s: command not found\n",inputBuffer->input);
    return false;
}