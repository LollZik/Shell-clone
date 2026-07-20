#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"


Command dispatch_table[] = {
    {"echo", cmd_echo},
    {"exit", cmd_exit},
    {"type", cmd_type}
};

const int num_commands = sizeof(dispatch_table) / sizeof(Command);

int cmd_echo(char** args) {
    int i = 1;
    while (args[i] != NULL) {
        printf("%s", args[i]);
        if (args[i+1] != NULL) {
            printf(" ");
        }
        i++;
    }
    printf("\n");
    
    return 0;
}

int cmd_exit(char** args) {
    exit(0);
    return 0; 
}

int cmd_type(char** args) {

    bool found = false;
    char* command = args[1];
    if (command == NULL) {return 0;}

    for(int i = 0; i < num_commands; i++){
      if(strcmp(command, dispatch_table[i].name) == 0){
        printf("%s is a shell builtin\n",command);
        found = true;
        break;
      }
    }
    if(!found){
      char* result = search_PATH(command);
      if(result == NULL){
        printf("%s: not found\n",command);
      }
      else{
        printf("%s is %s\n", command, result);
        free(result);
      }
    }
    return 0;
}

bool checkInput(InputBuffer *inputBuffer){
    char **args = tokenize_input(inputBuffer->input);

    if(args[0] == NULL) {
        free(args);
        return false;
    }

    for(int i = 0; i < num_commands; i++){
        if(strcmp(args[0], dispatch_table[i].name) == 0){
            inputBuffer->valid_input = true;
            dispatch_table[i].func(args);
            free(args);
            return true;
        }
    }
    if(inputBuffer->valid_input == false){
        char* filepath = search_PATH(args[0]);
        if(filepath != NULL){
            execute_file(args, filepath);
            free(filepath);
            free(args);
            return true;
        }
    }
    free(args);
    printf("%s: command not found\n",inputBuffer->input);
    return false;
}