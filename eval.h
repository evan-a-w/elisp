#ifndef EVAL_H
#define EVAL_H

#include "garb.h"
#include "hash_table.h"
#include "parser.h"

typedef char *error_t;

typedef struct {
    handle_t env;
    handle_t *stack;
    unsigned long stack_sz;
    unsigned long stack_cap;
} eval_state_t;

handle_t ast_to_val(ast_t *ast);

#endif
