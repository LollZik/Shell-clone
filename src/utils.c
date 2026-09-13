#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>

#include "utils.h"

// Global list of environment variables
extern char **environ;


InputBuffer create_input(){
  InputBuffer inputBuffer;

  inputBuffer.input = NULL;
  inputBuffer.capacity = 0;
  inputBuffer.input_size = 0;

  return inputBuffer;
}

bool capture_input(InputBuffer *inputBuffer){
  ssize_t chars_read = getline(&(inputBuffer->input), &(inputBuffer->capacity), stdin);

  if(chars_read == -1){
      return false; 
  }

  if(chars_read > 0 && inputBuffer->input[chars_read - 1] == '\n'){
      inputBuffer->input[chars_read - 1] = '\0';
      chars_read--;
  }
  inputBuffer->input_size = chars_read;

  return true;
}

#define START_WORD() \
    if (!inside_word) { \
        args[i++] = &arguments[write_idx]; \
        inside_word = true; \
    }

char** tokenize_input(char* arguments){

    if(arguments == NULL){
        return NULL;
    }

    typedef enum{
        NO_QUOTES,
        SINGLE_QUOTES,
        DOUBLE_QUOTES,
    } State;

    State state = NO_QUOTES;

    size_t capacity = 4;
    char** args = malloc(capacity * sizeof(char*));
    size_t i = 0;

    size_t read_idx = 0;
    size_t write_idx = 0;

    bool inside_word = false;

    while (arguments[read_idx] != '\0') {
        char c = arguments[read_idx];
        read_idx++;
        if (i >= capacity - 1) {
            capacity *= 2;
            args = realloc(args, capacity * sizeof(char*));
        }

        switch (state) {
            case NO_QUOTES:
                if (c == ' ' || c == '\t') {
                    if (inside_word) {
                        arguments[write_idx++] = '\0';
                        inside_word = false;
                    }
                } 
                else if (c == '\'') {
                    state = SINGLE_QUOTES;
                    START_WORD();
                } 
                else if (c == '\"') {
                    state = DOUBLE_QUOTES;
                    START_WORD();
                } 
                else if (c == '\\') {
                    char next_char = arguments[read_idx];
                    
                    if (next_char != '\0') {
                        START_WORD();
                        arguments[write_idx++] = next_char;
                        read_idx++;
                    }
                    else {
                        break; 
                    }
                } 
                else {
                    // Any regular letter
                    START_WORD();
                    arguments[write_idx++] = c;
                }
                break;

            case SINGLE_QUOTES:
                if (c == '\'') {
                    state = NO_QUOTES; 
                } else {
                    arguments[write_idx++] = c; 
                }
                break;

            case DOUBLE_QUOTES:
                if (c == '\"') {
                    state = NO_QUOTES; 
                } 
                else if (c == '\\') {
                    char next_char = arguments[read_idx];
                    
                    if (next_char == '\\' || next_char == '\"') {
                        arguments[write_idx++] = next_char;
                        read_idx++;

                    } else if (next_char != '\0') {
                        // If it was something like "\a" keep the '\'
                        arguments[write_idx++] = '\\';
                    }
                } 
                else {
                    arguments[write_idx++] = c;
                }
                break;
        }
    }
    if (inside_word) {
        arguments[write_idx++] = '\0';
    }
    args[i] = NULL;

    return args;
}

char *get_env(const char *__name){
    size_t len = strlen(__name);
    for(char **env = environ; *env != NULL; env++){
        if(strncmp(*env, __name,len)==0 && (*env)[len] == '='){
            return strdup(*env + len + 1);
        }
    }
    return NULL;
}

char* search_PATH(char *command){
  char *path = strdup(get_env("PATH"));
  if(path != NULL){
    char *dir = strtok(path,":");
    while(dir != NULL){
      char fullPath[1024];
      snprintf(fullPath, sizeof(fullPath), "%s/%s",dir,command);
      
      if(access_file(fullPath, X_OK) == 0){
        free(path);
        return strdup(fullPath);
      }
      dir = strtok(NULL,":");
    }
    free(path);
  }
  return NULL;
}

bool setup_redirections(char *out_file, char *err_file, bool out_append, bool err_append, int *saved_stdout, int *saved_stderr) {
    int fd_out = -1;
    int fd_err = -1;

    if (out_file != NULL) {
        int flags = O_WRONLY | O_CREAT | (out_append ? O_APPEND : O_TRUNC);
        fd_out = open(out_file, flags, 0644);
        if (fd_out < 0) {
            perror("shell");
            return false;
        }
    }

    if (err_file != NULL) {
        int flags = O_WRONLY | O_CREAT | (err_append ? O_APPEND : O_TRUNC);
        fd_err = open(err_file, flags, 0644);
        if (fd_err < 0) {
            perror("shell");
            if (fd_out != -1) close(fd_out);
            return false;
        }
    }

    if (fd_out != -1) {
        if (saved_stdout != NULL) {
            *saved_stdout = dup(STDOUT_FILENO);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    if (fd_err != -1) {
        if (saved_stderr != NULL) {
            *saved_stderr = dup(STDERR_FILENO);
        }
        dup2(fd_err, STDERR_FILENO);
        close(fd_err);
    }

    return true;
}

void restore_redirections(int saved_stdout, int saved_stderr) {
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    if (saved_stderr != -1) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }
}

bool execute_file(char** args, char* filepath, char* out_file, char* err_file, bool out_append, bool err_append){
    pid_t pid = fork();

    if (pid < 0) {
        return false;
    }

    else if (pid == 0){
        if (!setup_redirections(out_file, err_file, out_append, err_append, NULL, NULL)) {
            exit(EXIT_FAILURE);
        }

        execv(filepath, args);

        // This code executes only if execv fails
        perror("execv failed");
        exit(EXIT_FAILURE);
    }

    else{
        wait(NULL);
    }
    return true;
}

static inline uid_t get_uid(void){
    return (uid_t) syscall(SYS_getuid);
}

static inline gid_t get_gid(void){
    return (gid_t) syscall(SYS_getgid);
}

static int check_mode_bit(mode_t st_mode, uid_t st_uid, gid_t st_gid, uid_t ruid, gid_t rgid,
    gid_t *groups, int ngroups, mode_t owner_bit, mode_t group_bit, mode_t other_bit){
        
        // Special case for root
        if(ruid == 0){
            if(owner_bit == (mode_t)0 && group_bit == (mode_t)0 && other_bit == (mode_t)0){ // If checking for existence only, root has access
                return 1;
            }
            if ((owner_bit & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0){ // If we are checking for the execution privilege
                if(st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)){  // If anyone has execution privileges, root also has access
                    return 1;
                }
                else{
                    return 0;
                }
            }
            else{
                return 1; // R/W is allowed for root in this (simplified) model
            }
        }

        if(ruid == st_uid){ // If user is the file's owner
            return (st_mode & owner_bit) ? 1 : 0;
        }

        if(rgid == st_gid){ // If user is among the file's group
            return (st_mode & group_bit) ? 1 : 0;
        }

        // Check for supplementary groups
        for(int i = 0 ; i < ngroups ; i++){
            if(groups[i] == st_gid){
                return (st_mode & group_bit) ? 1 : 0;
            }
        }

        // Check for "other"  privileges
        return (st_mode & other_bit) ? 1 : 0;


}

static int check_path_prefix_search(const char *path, uid_t ruid, gid_t rgid, gid_t *groups, int ngroups){
    size_t n = strlen(path);

    if(n == 0){
        errno = ENOENT;
        return -1;
    }

    if(n > PATH_MAX_LEN){
        errno = ENAMETOOLONG; 
        return -1;
    }

    char buf[PATH_MAX_LEN+1];
    strncpy(buf, path, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
    
    size_t i = 0;
    // Check if each file along the path is directory and is searchable (execute/search permission)
    // If absolute path, check root directory first
    if(buf[0] == '/'){
        struct stat st;
        if(stat("/", &st) != 0 ){
            return -1;
        }

        // Check if caller can search the root directory
        if(!check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ngroups, S_IXUSR, S_IXGRP, S_IXOTH)){
            errno = EACCES;
            return -1;
        }
        while(buf[i] == '/') i++; // Skip over repeated '/'
    }
    else{
        struct stat st;
        if(stat(".", &st) != 0){
            return -1;
        }
        if(!check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ngroups, S_IXUSR, S_IXGRP, S_IXOTH)){
            errno = EACCES;
            return -1;
        }
        i = 0;
    }

        // Iterate remaining path components and check each prefix that ends with '/'
    for(; i < n; i++){
        if(buf[i] == '/'){
            // Temporarily terminate string at the '/' to form a prefix 
            buf[i] = '\0';
            if(buf[0] == '\0'){ // If prefix is empty restore and continue
                buf[i] = '/';
                continue;
            }
            struct stat st;
            if(stat(buf, &st) != 0){
                return -1;
            }
            if(!S_ISDIR(st.st_mode)){
                errno = ENOTDIR;
                return -1;
            }
            // Check search (execute) bit for this directory
            if(!check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ngroups, S_IXUSR, S_IXGRP, S_IXOTH)){
                errno = EACCES;
                return -1;
            }
            // Restore the '/' and skip repeated slashes
            buf[i] = '/';
            while(buf[i] == '/' && i < n) i++;
            --i; // for loop will do ++i
        }
    }
    return 0;
}

int access_file(const char *path, int type){
    if(!path){
        errno = EINVAL;
        return -1;
    }

    uid_t ruid = get_uid();
    gid_t rgid = get_gid();

    // Get supplementary groups, then allocate and fetch them
    int ng = getgroups(0, NULL);
    gid_t *groups = NULL;
    if(ng > 0){
        groups = malloc(sizeof(gid_t) * ng);
        if(!groups){
            errno = ENOMEM;
            return -1;
        }
        if(getgroups(ng, groups) < 0){
            free(groups);
            return -1;
        }
    }
    else{
        ng = 0;
    }

    // Ensure we can traverse the directory components (search permission)
    if(check_path_prefix_search(path, ruid, rgid, groups, ng) != 0){
        free(groups);
        return -1; 
    }

    struct stat st;
    if(stat(path, &st) != 0){
        free(groups);
        return -1;
    }

    // Check if file exists
    if( type == F_OK){
        free(groups);
        return 0;
    }

    // Check other permissions if requested 
    if(type & R_OK){
        int ok = check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ng, S_IRUSR, S_IRGRP, S_IROTH);
        if(!ok){
            free(groups);
            errno = EACCES;
            return -1;
        }
    }
    if(type & W_OK){
        int ok = check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ng, S_IWUSR, S_IWGRP, S_IWOTH);
        if(!ok){
            free(groups);
            errno = EACCES;
            return -1;
        }
    }
    if(type & X_OK){
        int ok = check_mode_bit(st.st_mode, st.st_uid, st.st_gid, ruid, rgid, groups, ng, S_IXUSR, S_IXGRP, S_IXOTH);
        if(!ok){
            free(groups);
            errno = EACCES;
            return -1;
        }
    }

    free(groups);
    return 0;
}
