#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include <stdbool.h>

#define MAX_BUFFER_SIZE 128

typedef struct {
  char *input;
  uint8_t input_size;
  bool valid_input;
} InputBuffer;


InputBuffer createInput(void);

uint8_t captureInput(InputBuffer *inputBuffer);

bool searchPATH(char *command);

/* Return a duplicate of the value of the environment variable NAME, or NULL if it doesn't exist.  */
extern char *get_env (const char *__name);

/* Test for access to the specified path using the real UID and real GID.  */
extern int access_file (const char *path, int type);

#endif