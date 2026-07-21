#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"


Command dispatch_table[] = {
    {"echo", cmd_echo },
    {"exit", cmd_exit },
    {"type", cmd_type },
    {"pwd",  cmd_pwd  },
    {"cd",   cmd_cd   },
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

int cmd_pwd(char** args){
    char cwd[PATH_MAX_LEN]; 
    
    if (getcwd(cwd, sizeof(cwd) ) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd failed"); 
    }
    
    return 0;
}

int cmd_cd(char** args){
    char* path = args[1];

    if (path == NULL) {
        char* home = get_env("HOME");
        if (home != NULL) {
            chdir(home);
            free(home);
        }
        return 0;
    }

    if (path[0] == '~') {
        char* home = get_env("HOME");
        
        if (home != NULL) {
            char full_path[4096];
            snprintf(full_path, sizeof(full_path), "%s%s", home, path + 1);
            
            if (chdir(full_path) != 0) {
                printf("cd: %s: No such file or directory\n", path);
            }
            free(home);
        }
        return 0;
    }

    if (chdir(path) != 0) {
        printf("cd: %s: No such file or directory\n", path);
    }
    return 0;
}

bool handle_input(InputBuffer *inputBuffer){
    char **args = tokenize_input(inputBuffer->input);

    if(args[0] == NULL) {
        free(args);
        return false;
    }

    for(int i = 0; i < num_commands; i++){
        if(strcmp(args[0], dispatch_table[i].name) == 0){
            dispatch_table[i].func(args);
            free(args);
            return true;
        }
    }
    char* filepath = search_PATH(args[0]);
    if(filepath != NULL){
        execute_file(args, filepath);
        free(filepath);
        free(args);
        return true;
    }
    free(args);
    printf("%s: command not found\n",inputBuffer->input);
    return false;
}