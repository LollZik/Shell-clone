#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // getuid, getgid, getgroups
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

// Global list of environment variables
extern char **environ;

char *get_env(const char *__name){
    size_t len = strlen(__name);
    for(char **env = environ; *env != NULL; env++){
        if(strncmp(*env, __name,len)==0 && (*env)[len] == '='){
            return strdup(*env + len + 1);
        }
    }
    return NULL;
}

int access_file(const char *__name, int __type){
    /*
    __type:
    4 -> test for read permission
    2 -> test for write permission
    1 -> test for execute permission
    0 -> test for existence
    */

    
}