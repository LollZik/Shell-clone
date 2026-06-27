#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "commands.h"


int main(int argc, char *argv[]){

  setbuf(stdout, NULL);
  printf("$ ");
  InputBuffer inputBuffer = createInput();

  while(captureInput(&inputBuffer)){
    checkInput(&inputBuffer);
    printf("$ ");
  }
  return EXIT_SUCCESS;
}
