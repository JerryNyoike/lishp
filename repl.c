#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

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

int number_of_nodes(mpc_ast_t* t){
  if (t->children_num == 0) return 1;
  return 1 + number_of_nodes(*(t->children + 1));
}

/* Declare a lisp val struct to help with error handling */
typedef struct {
  int type;
  long num;
  int err;
} lval;

/* Enum of possible lisp val types */
enum { LVAL_NUM, LVAL_ERR };

/* Enum of possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* lval constructors */
lval lval_num(long x) {
  lval val;
  val.type = LVAL_NUM;
  val.num = x;
  return val;
}

lval lval_err(int e) {
  lval val;
  val.type = LVAL_ERR;
  val.err = e;
  return val;
}

/* Print a lisp value */
void lval_print(lval val) {
  switch (val.type) {
  case LVAL_NUM:
    printf("%ld\n", val.num);
    break;
  case LVAL_ERR:
    if (val.err == LERR_DIV_ZERO)
      printf("Error: Division by zero!");
    else if (val.err == LERR_BAD_OP)
      printf("Error: Unsupported operator!");
    else if (val.err == LERR_BAD_NUM)
      printf("Error: Invalid number!");
    break;
  }
}

void lval_println(lval val) { lval_print(val); putchar('\n'); }

lval eval(mpc_ast_t* tree);
lval eval_bin_op(char* op, lval first_arg, lval second_arg);
long eval_op(char* op, long arg);

int main(int argc, char** argv){
  /* Define the grammar for polish notation. */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                           \
              number   : /-?[0-9]+/ ;					\
              operator : '+' | '-' | '*' | '/' | '%';			\
              expr     : <number> | '(' <operator> <expr>+ ')';		\
              lispy    : /^/ <operator> <expr>+ /$/;			\
	    ",
	    Number, Expr, Operator, Lispy);
  
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
    /* printf("You gave me %s\n", input); */
    /* Parse the user input */
    mpc_result_t result;

    if (mpc_parse("<stdin>", input, Lispy, &result)) {
      /* Print the AST. */
      lval res = eval(result.output);
      lval_println(res);
      /* printf("%li\n", res); */
      /* mpc_ast_print(result.output); */
      /* mpc_ast_delete(result.output); */
    } else {
      /* Handle the error */
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }
    /* Free retrieved input. */
    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  return 0;
}

lval eval(mpc_ast_t* tree) {
  if (strstr(tree->tag, "number")) {
    /* Check that number is within range */
    errno = 0;
    long num = strtol(tree->contents, NULL, 10);
    return errno != ERANGE ? lval_num(num) : lval_err(LERR_BAD_NUM);
  }

  /* Get the operator */
  char* op = tree->children[1]->contents;

  /* Evaluate the first operand  */
  lval res = eval(tree->children[2]);

  /* Reduce the entire expression */
  int i = 3;
  while (strstr(tree->children[i]->tag, "expr")) {
    res = eval_bin_op(op, res, eval(tree->children[i++]));
  }

  return res;
}

lval eval_bin_op(char* op, lval first_arg, lval second_arg) {
  /* Check if either value is an error */
  if (first_arg.type == LVAL_ERR) return first_arg;
  if (second_arg.type == LVAL_ERR) return second_arg;
  
  if (strcmp(op, "+") == 0) return lval_num(first_arg.num + second_arg.num); 
  else if (strcmp(op, "-") == 0) return lval_num(first_arg.num - second_arg.num); 
  else if (strcmp(op, "*") == 0) return lval_num(first_arg.num * second_arg.num); 
  else if (strcmp(op, "%") == 0) return lval_num(first_arg.num % second_arg.num);
  else if (strcmp(op, "/") == 0) {
    if (second_arg.num == 0)    /* Second number is zero return error */
      return lval_err(LERR_DIV_ZERO);
    return lval_num(first_arg.num / second_arg.num);
  }

  /* The operator does not match any of the supported ones, return error */
  return lval_err(LERR_BAD_OP);
}

long eval_op(char* op, long arg) {
  if (strcmp(op, "+") == 0) return arg;
  else if (strcmp(op, "-") == 0) return -arg;
  return 0;
}
