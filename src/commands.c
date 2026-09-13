#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

    char *out_file = NULL;
    char *err_file = NULL;
    bool out_append = false;
    bool err_append = false;
    int cut_idx = -1;

    if(args[0] == NULL) {
        free(args);
        return false;
    }

    for(int i =0; args[i] != NULL; i++){
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], "1>") == 0) {
            out_file = args[i+1];
        }
        else if (strcmp(args[i], ">>") == 0 || strcmp(args[i], "1>>") == 0) {
            out_file = args[i+1];
            out_append = true;
        }
        else if (strcmp(args[i], "2>") == 0) {
            err_file = args[i+1];
        }
        else if (strcmp(args[i], "2>>") == 0) {
            err_file = args[i+1];
            err_append = true;
        }
        else {
            continue;
        }

        if (cut_idx == -1) {
        cut_idx = i;
        }
    }

    if(cut_idx != -1){
        args[cut_idx] = NULL;
    }

    for (int i = 0; i < num_commands; i++) {
        if (strcmp(args[0], dispatch_table[i].name) == 0) {
            int saved_stdout = -1;
            int saved_stderr = -1;

            if (!setup_redirections(out_file, err_file, out_append, err_append, &saved_stdout, &saved_stderr)) {
                free(args);
                return true;
            }

            dispatch_table[i].func(args);

            restore_redirections(saved_stdout, saved_stderr);

            free(args);
            return true;
        }
    }

    char* filepath = search_PATH(args[0]);
    if (filepath != NULL) {
        execute_file(args, filepath, out_file, err_file, out_append, err_append);
        free(filepath);
        free(args);
        return true;
    }

    printf("%s: command not found\n", inputBuffer->input);
    free(args);
    return false;
}