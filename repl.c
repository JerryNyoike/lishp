#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

/* Declare a buffer for user input of size 2048. */
static char buffer[2048];

char* readline(char* prompt) {
  /* Output the prompt. */
  fputs(prompt, stdout);

  /* Get the input and store it in the input buffer. */
  fgets(input, 2048, stdin);

  /* Makea copy of the input and return it. */
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Add a fake add_history function. */
void add_history(char* unused);

/* Otherwise add the required headers. */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

int main(int argc, char** argv){
  /* Print Version and exit information. */
  puts("Lispy Version 0.0.1");
  puts("Press Ctrl+c to exit.");
  puts("Press Ctrl+d gives a segfault, we are working to fix this.");

  /* In a never ending loop. */
  while (1) {
    /* Output the prompt */
    char* input = readline("Lishp> ");

    /* Add line to history. */
    add_history(input);

    /* Echo input back to user */
    printf("You gave me %s\n", input);

    /* Free retrieved input. */
    free(input);
  }

  return 0;
}
