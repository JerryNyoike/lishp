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
typedef struct lval {
  int type;
  long num;
  char* err;
  char* sym;
  int count;
  struct lval** cell;
} lval;

/* Enum of possible lisp val types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

/* lval constructors */
lval* lval_num(long x);
lval* lval_err(char* e);
lval* lval_sym(char* sym);
lval* lval_sexpr(void);

/* Lisp value destructor */
void lval_del(lval* val);

/* Print a lisp value */
void lval_print(lval* val);
void lval_println(lval* val);
void lval_expr_print(lval* val, char open, char close);

/* Read functions */
lval* lval_read_num(mpc_ast_t* tree);
lval* lval_read(mpc_ast_t* tree);
lval* lval_add(lval* val, lval* new_val);

/* Eval functions */
lval* builtin_op(char* sym, lval* sexpr);
lval* lval_take(lval* val, int i);
lval* lval_pop(lval* sexpr, int i);
lval* lval_eval(lval* val);
lval* lval_eval_sexpr(lval* sexpr);
lval eval_bin_op(char* op, lval first_arg, lval second_arg);
long eval_op(char* op, long arg);

int main(int argc, char** argv){
  /* Define the grammar for polish notation. */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Lispy = mpc_new("lispy");
  mpc_parser_t* Sexpr = mpc_new("sexpr");

  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                           \
              number   : /-?[0-9]+/;					\
              symbol   : '+' | '-' | '*' | '/' | '%';			\
	      sexpr    : '(' <expr>* ')';				\
              expr     : <number> | <symbol> | <sexpr>;			\
              lispy    : /^/ <expr>* /$/;				\
	    ",
	    Number, Symbol, Sexpr, Expr, Lispy);
  
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
      lval* res = lval_eval(lval_read(result.output));

      lval_println(res);
      lval_del(res);
    } else {
      /* Handle the error */
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }
    /* Free retrieved input. */
    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
  return 0;
}

lval* lval_num(long num) {
  lval* val = malloc(sizeof(lval));
  val->type = LVAL_NUM;
  val->num = num;
  return val;
}

lval* lval_err(char* err) {
  lval* val = malloc(sizeof(lval));
  val->type = LVAL_ERR;
  val->err = malloc(strlen(err) + 1);
  strcpy(val->err, err);
  return val;
}

lval* lval_sym(char* sym) {
  lval* val = malloc(sizeof(lval));
  val->type = LVAL_SYM;
  val->sym = malloc(strlen(sym) + 1);
  strcpy(val->sym, sym);
  return val;
}

lval* lval_sexpr(void) {
  lval* val = malloc(sizeof(lval));
  val->type = LVAL_SEXPR;
  val->count = 0;
  val->cell = NULL;
  return val;
}

void lval_del(lval* val) {
  switch (val->type) {
  case LVAL_NUM: break;
  case LVAL_SYM:
    free(val->sym);
    break;
  case LVAL_ERR:
    free(val->err);
    break;
  case LVAL_SEXPR:
    for (int i = 0; i < val->count; i++) {
      lval_del(val->cell[i]);
    }
    free(val->cell);
    break;
  }

  free(val);
}

void lval_print(lval* val) {
  switch (val->type) {
  case LVAL_NUM:
    printf("%ld", val->num);
    break;
  case LVAL_ERR:
    printf("Error: %s!", val->err);
    break;
  case LVAL_SYM:
    printf("%s", val->sym);
    break;
  case LVAL_SEXPR:
    lval_expr_print(val, '(', ')');
    break;
  }
}

void lval_println(lval* val) { lval_print(val); putchar('\n'); }

void lval_expr_print(lval* val, char open, char close) {
  putchar(open);

  for (int i = 0; i < val->count; i++) { /* Print the value */
    lval_print(val->cell[i]);
    
    if (i != val->count - 1) {	/* Print a space between the values */
      putchar(' ');
    }
  }

  putchar(close);
}

lval* lval_read_num(mpc_ast_t* tree) {
  errno = 0;
  long num = strtol(tree->contents, NULL, 10);
  return errno != ERANGE ?
    lval_num(num) : lval_err("invalid number!");
}

lval* lval_read(mpc_ast_t* tree) {
  /* If it's a symbol/number return the symbol/number  */
  if (strstr(tree->tag, "number")) return lval_read_num(tree);
  if (strstr(tree->tag, "symbol")) return lval_sym(tree->contents);

  /* If at the root or start of a new sexpr, create a new sexpr */
  lval* sexpr = NULL;

  if (strcmp(tree->tag, ">") == 0) { sexpr = lval_sexpr(); }
  if (strstr(tree->tag, "sexpr")) { sexpr = lval_sexpr(); }

  /* Add all valid expressions contained within the parentheses. */
  for (int i = 0; i < tree->children_num; i++) {
    if (strcmp(tree->children[i]->contents, "(") == 0) continue;
    if (strcmp(tree->children[i]->contents, ")") == 0) continue;
    if (strcmp(tree->children[i]->tag, "regex") == 0) continue;

    sexpr = lval_add(sexpr, lval_read(tree->children[i]));
  }

  return sexpr;
}

lval* lval_add(lval* orig_lval, lval* new_lval) {
  orig_lval->count++;
  orig_lval->cell = realloc(orig_lval->cell, sizeof(lval*) * orig_lval->count);
  orig_lval->cell[orig_lval->count - 1] = new_lval;
  return orig_lval;
}

lval* lval_eval_sexpr(lval* sexpr) {
  /* Evaluate the children */
  for (int i = 0; i < sexpr->count; i++) {
    sexpr->cell[i] = lval_eval(sexpr->cell[i]);
  }

  /* In case of an error return it */
  for (int i = 0; i < sexpr->count; i++) {
    if (sexpr->cell[i]->type == LVAL_ERR) {
      return lval_take(sexpr, i);
    }
  }

  /* In case of an empty sexpr return it */
  if (sexpr->count == 0) {
    return sexpr;
  }

  /* In case of a single element sexpr return the single element */
  if (sexpr->count == 1) {
    return lval_take(sexpr, 0);
  }

  /* Ensure the first element is a symbol */
  lval* first = lval_pop(sexpr, 0);
  if (first->type != LVAL_SYM) {
    lval_del(first);
    lval_del(sexpr);
    return lval_err("S-Expression does not start with symbol");
  }
  
  /* apply a builtin to the reduced sexpr */
  lval* result = builtin_op(first->sym, sexpr);
  lval_del(first);
  return result;
}

lval* lval_eval(lval* sexpr) {
  if (sexpr->type == LVAL_SEXPR) /* reduce s-expressions then return them */
    return lval_eval_sexpr(sexpr);

  return sexpr;			/* Return all other types as they're in simplest form */
}

lval* lval_pop(lval* sexpr, int i) { /* Get the element at i and remove it from the sexpr */
  lval* elem = sexpr->cell[i];

  memmove(&sexpr->cell[i], &sexpr->cell[i+1],  /* Move memory over the top */
	  sizeof(lval*) * (sexpr->count-i-1));

  sexpr->count--;		/* Decrease count of items */
  sexpr->cell = realloc(sexpr->cell, sizeof(lval*) * sexpr->count); /* Realloc the memory used */
  return elem;			/* Return the element popped */
}

lval* lval_take(lval* sexpr, int i) {		/* Like lval_pop but deletes the list from which it was taken */
  lval* elem = lval_pop(sexpr, i);
  lval_del(sexpr);
  return elem;
}

lval* builtin_op(char* op, lval* operands) {
  /* Check that all operands are numbers */
  for (int i = 0; i < operands->count; i++) {
    if (operands->cell[i]->type != LVAL_NUM) {
      lval_del(operands);
      return lval_err("Cannot operate on a non-number");
    }
  }

  lval* first_arg = lval_pop(operands, 0);

  if (strcmp(op, "-") == 0 && operands->count == 0) { /* Negate the number */
    first_arg->num = -first_arg->num;
    return first_arg;
  }

  while (operands->count > 0) {
    /* Pop the next element */
    lval* second_arg = lval_pop(operands, 0);

    if (strcmp(op, "+") == 0) first_arg->num += second_arg->num;
    else if (strcmp(op, "-") == 0) first_arg->num -= second_arg->num;
    else if (strcmp(op, "*") == 0) first_arg->num *= second_arg->num;
    else if (strcmp(op, "%") == 0) first_arg->num %= second_arg->num;
    else if (strcmp(op, "/") == 0) {
      if (second_arg->num == 0)    /* Second number is zero return error */
	return lval_err("Division by zero");
      first_arg->num /= second_arg->num;
    }

    /* Delete the second argument as we don't need it. */
    lval_del(second_arg);
  }

  lval_del(operands);
  return first_arg;
}
