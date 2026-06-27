#include "utils.h"

typedef int (*CommandFunc)(InputBuffer *input);

bool checkInput(InputBuffer *inputBuffer);


typedef struct {
    char        *name; 
    CommandFunc func;
} Command;