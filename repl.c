#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

/* Declare a buffer for user input of size 2048. */
char input[2048];

int main(int argc, char** argv){
  /* Print Version and exit information. */
  puts("Lispy Version 0.0.1");
  puts("Press Ctrl+c to exit.");

  /* In a never ending loop. */
  while (1) {
    /* Output the prompt */
    char* input = readline("Lispy> ");
    /* fputs("Lispy> ", stdout); */

    /* Read a line of user input of max size 2048 */
    /* fgets(input, 2048, stdin); */

    /* Add line to history. */
    add_history(input);

    /* Echo input back to user */
    printf("No you are a %s\n", input);

    /* Free retrieved input.  */
    free(input);
  }

  return 0;
}
