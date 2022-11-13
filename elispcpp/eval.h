#ifndef EVAL_H
#define EVAL_H

#include <vector>

#include "garb.h"
#include "parser.h"
#include "env.h"

handle_t ast_to_val(ast_t *ast);
handle_t eval_ast(ast_t *);
handle_t eval(void);
void print_value(handle_t h);

#endif
