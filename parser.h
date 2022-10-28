#ifndef PARSER_H
#define PARSER_H

#include "tokeniser.h"

typedef enum ast_type {
    AstError,
    AtomInt,
    AtomIdent,
    AtomString,
    SExpr,
} ast_type_t;

typedef struct sexpr {
    struct ast *val;
    struct sexpr *next;
} sexpr_t;

typedef struct ast {
    ast_type_t type; 
    union {
        char *s;
        long long i;
        sexpr_t *l;
    };
} ast_t;

typedef struct program {
    ast_t *ast;
    struct program *next;
} program_t;

void free_ast(ast_t *ast);
void free_sexpr(sexpr_t *sexpr);
void free_program(program_t *program);
ast_t *parse_one(tokeniser_t *tokeniser);
program_t *parse(tokeniser_t *tokeniser);
void print_ast(FILE *f, ast_t *ast);
void print_program(FILE *f, program_t *program);

#endif
