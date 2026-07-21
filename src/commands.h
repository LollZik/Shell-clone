#ifndef COMMANDS_H
#define COMMANDS_H

#include "utils.h"

typedef int (*CommandFunc)(char **args);

bool handle_input(InputBuffer *inputBuffer);


typedef struct {
    char        *name; 
    CommandFunc func;
} Command;

int cmd_echo(char **args);
int cmd_exit(char **args);
int cmd_type(char **args);
int cmd_pwd(char **args);
int cmd_cd(char **args);

#endif