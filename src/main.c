#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "utils.h"

#define MAX_BUFFER_SIZE 128

char *commands[] = {
  "echo",
  "exit",
  "type"
};

typedef struct {
  char *input;
  uint8_t input_size;
  bool valid_input;
} InputBuffer;


InputBuffer createInput(){
  InputBuffer inputBuffer;

  inputBuffer.input = calloc(MAX_BUFFER_SIZE, sizeof(char));
  inputBuffer.input_size = 0;
  inputBuffer.valid_input = false;

  return inputBuffer;
}

uint8_t captureInput(InputBuffer *inputBuffer){
  fgets(inputBuffer->input, MAX_BUFFER_SIZE, stdin);
  inputBuffer->input_size = strlen(inputBuffer->input) - 1;
  inputBuffer->input[inputBuffer->input_size] = '\0';

  return inputBuffer->input_size;
}

bool searchPATH(char *command){
  char *path = strdup(get_env("PATH"));
  if(path != NULL){
    char *dir = strtok(path,":");
    while(dir != NULL){
      char fullPath[1024];
      snprintf(fullPath, sizeof(fullPath), "%s/%s",dir,command);
      if(access_file(fullPath, 1) == 0){
        printf("%s is %s\n",command, fullPath);
      }
      dir = strtok(NULL,":");
    }
    free(path);
  }
  return false;
}


bool checkInput(InputBuffer *inputBuffer){
  char *command = strtok(inputBuffer->input, " "); // Separate input into words
  if(command == NULL){return false;}

  if(strcmp(command,"exit") == 0){
    exit(EXIT_SUCCESS);
  }
  else if(strcmp(command, "echo") == 0){

    inputBuffer->valid_input = true;
    printf("%s\n",inputBuffer->input + strlen(command)+1); // Print whole input besides the "echo "
  }
  else if(strcmp(command, "type") == 0){
    inputBuffer->valid_input = true;
    bool found = false;
    command = strtok(NULL, " "); // get the second word from input

    for(int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++){
      if(strcmp(command,commands[i])==0){
        printf("%s is a shell builtin\n",command);
        found = true;
        break;
      }
    }
    if(!found){
      found = searchPATH(command);
      if (!found){
        printf("%s: not found\n",command);
      }
    }
  }
  else{
    inputBuffer->valid_input = false;
  };
  return inputBuffer->valid_input;
}

int main(int argc, char *argv[]){

  setbuf(stdout, NULL);
  printf("$ ");
  InputBuffer inputBuffer = createInput();

  while(captureInput(&inputBuffer)){

    checkInput(&inputBuffer);
    if(!inputBuffer.valid_input){
      printf("%s: command not found\n",inputBuffer.input);
    }
    printf("$ ");
  }
  return EXIT_SUCCESS;
}
