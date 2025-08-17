#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
  while(true){
    setbuf(stdout, NULL);
    printf("$ ");

    char input[100];
    fgets(input, 100, stdin);
    input[strlen(input)-1] = '\0';

    if(strcmp(input, "exit 0") == 0){
      return 0;
    }
    else if(strncmp(input,"echo ",5) == 0){
      printf("%s\n",input + 5);
    }
    else{
      printf("%s: command not found\n", input);
    }
}
  return 0;
}
