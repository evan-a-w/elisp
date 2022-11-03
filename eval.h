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
} eval_state_t;

handle_t ast_to_val(ast_t *ast);
handle_t eval_ast(eval_state_t *, ast_t *);
handle_t st_push(eval_state_t *, handle_t);
handle_t st_pop(eval_state_t *);
void st_free(eval_state_t *);
eval_state_t st_new(void);
handle_t eval(eval_state_t *st);

#endif
