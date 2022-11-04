#ifndef EVAL_H
#define EVAL_H

#include "garb.h"
#include "hash_table.h"
#include "parser.h"
#include "vec.h"

typedef char *error_t;

typedef struct {
    handle_t env;
    ul_vec *st;
    ul_vec *res;
    ul to_eval;
} eval_state_t;

void call_on_st_vars(void (*f)(handle_t));
void st_init(void);
void st_destroy(void);
handle_t ast_to_val(ast_t *ast);
handle_t eval_ast(ast_t *);
handle_t st_push(handle_t);
handle_t st_pop(void);
handle_t st_rpop(void);
handle_t st_rpush(handle_t);
handle_t st_peek(void);
handle_t st_rpeek(void);
void st_free(eval_state_t *);
eval_state_t st_new(void);
handle_t eval(void);
handle_t st_push_dep(handle_t h);
void print_value(handle_t h);
void print_value(handle_t h);

#endif
