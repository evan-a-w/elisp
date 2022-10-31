#ifndef PARSER_H
#define PARSER_H

#include "tokeniser.h"

typedef enum ast_type {
    AstError,
    AtomInt,
    AtomBool,
    AtomString,
    AtomSymbol,
    SExpr,
    SpecialForm,
} ast_type_t;

typedef enum special_form {
    If,
    Let,
    Quote,
    Var,
} special_form_t;

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
        special_form_t special;
        bool b;
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
void debug_print_ast(ast_t *ast);

#endif
