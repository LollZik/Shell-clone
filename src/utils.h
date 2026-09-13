#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include <stdbool.h>

#ifndef PATH_MAX_LEN
#define PATH_MAX_LEN 4096
#endif

typedef struct {
  char *input;
  size_t capacity;
  size_t input_size;
} InputBuffer;

InputBuffer create_input(void);
bool capture_input(InputBuffer *inputBuffer);

/* Split a raw input string into a dynamically allocated, NULL-terminated array of arguments. */
char** tokenize_input(char* arguments);

/* Return a duplicate of the value of the environment variable NAME, or NULL if it doesn't exist. */
char *get_env (const char *__name);

/* Search the PATH environment variable for the specified command, returning its full path or NULL if not found. */
char* search_PATH(char *command);

/* Redirect standard output and error to the specified files, optionally saving the original file descriptors. */
bool setup_redirections(char *out_file, char *err_file, bool out_append, bool err_append, int *saved_stdout, int *saved_stderr);

/* Restore the original standard output and error streams from the previously saved file descriptors. */
void restore_redirections(int saved_stdout, int saved_stderr);

/* Fork a new process and execute the specified file with the given arguments, waiting for its completion. */
bool execute_file(char** args, char* filepath, char* out_file, char* err_file, bool out_append, bool err_append);


/* Test for access to the specified path using the real UID and real GID. */
int access_file (const char *path, int type);

#endif