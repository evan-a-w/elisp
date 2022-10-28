#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tokeniser.h"
#include "parser.h"
#include "lib.h"

bool error_token(token_t *t) {
    return t->tok == Error || t->tok == None;
}

ast_t *make_ast(ast_t v) {
    ast_t *new_ast = malloc(sizeof(ast_t));
    *new_ast = v;
    return new_ast;
}

ast_t *tok_to_error_ast(token_t *t) {
    assert(t->tok == Error || t->tok == None);

    if (t->tok == Error) {
        return make_ast((ast_t) { .type = AstError, .s = t->s });
    } else {
        return make_ast((ast_t) { .type = AstError, .s = allocate_string("Expected token, received none") });
    }
}

void free_sexpr(sexpr_t *l) {
    if (l != NULL) {
        free_ast(l->val);
        free_sexpr(l->next);
    }
}

void free_program(program_t *l) {
    if (l != NULL) {
        free_ast(l->ast);
        free_program(l->next);
    }
}

void free_ast(ast_t *ast) {
    switch (ast->type) {
    case AstError:
    case AtomString:
    case AtomIdent:
        free(ast->s);
        break;
    case SExpr:
        free_sexpr(ast->l);
        break;
    default:
        break;
    }
}

ast_t *parse_inside_sexpr(tokeniser_t *t) {
    token_t curr_token;
    ast_t *res = malloc(sizeof *res);
    sexpr_t *s = malloc(sizeof *s);
    *res = (ast_t) { .type = SExpr, .l = s };
    *s = (sexpr_t) { .val = NULL, .next = NULL };
    for (;;) {
        curr_token = peek_token(t);
        if (error_token(&curr_token)) {
            free_ast(res);
            return tok_to_error_ast(&curr_token);
        } else if (curr_token.tok == RParen) {
            return res;
        }

        ast_t *next = parse_one(t);
        if (next->type == AstError) {
            free_ast(res);
            return next;
        }

        if (s->val == NULL) {
            s->val = next;
        } else {
            sexpr_t *new_s = malloc(sizeof *new_s);
            *new_s = (sexpr_t) { .val = next, .next = NULL };
            s->next = new_s;
            s = new_s;
        }
    }
}

ast_t *parse_one(tokeniser_t *t) {
    token_t curr_token = next_token(t);
    if (error_token(&curr_token)) {
        return tok_to_error_ast(&curr_token);
    }

    switch (curr_token.tok) {
    case LParen:
        return parse_inside_sexpr(t);
    case Ident:
        return make_ast((ast_t) { .type = AtomIdent, .s = curr_token.s });
    case Str:
        return make_ast((ast_t) { .type = AtomString, .s = curr_token.s });
    case Int:
        return make_ast((ast_t) { .type = AtomInt, .i = curr_token.i });
    default:
    {
        char *token = token_to_string(&curr_token);
        char *fst = "Unexpected token: ";
        char *res = malloc(strlen(fst) + strlen(token) + 1);
        strcat(res, fst);
        strcat(res, token);
        free(token);
        return make_ast((ast_t) { .type = AstError, .s = res });
    }
    }
}

program_t *parse(tokeniser_t *t) {
    program_t *res = NULL;
    while (!is_eof(t)) {
        ast_t *next = parse_one(t);
        if (next->type == AstError) {
            printf("Error in parsing: %s\n", next->s);
            free_ast(next);
            free_program(res);
            return NULL;
        }

        program_t *new_p = malloc(sizeof *new_p);
        *new_p = (program_t) { .ast = next, .next = NULL };
        if (res == NULL) {
            res = new_p;
        } else {
            res->next = new_p;
        }
    }
    return res;
}

void print_ast(FILE *f, ast_t *ast) {
    switch (ast->type) {
    case AstError:
        printf("Error: %s", ast->s);
        break;
    case AtomString:
        printf("\"%s\"", ast->s);
        break;
    case AtomIdent:
        printf("%s", ast->s);
        break;
    case AtomInt:
        printf("%lld", ast->i);
        break;
    case SExpr:
        printf("(");
        for (sexpr_t *s = ast->l; s != NULL; s = s->next) {
            print_ast(f, s->val);
            if (s->next != NULL) {
                printf(" ");
            }
        }
        printf(")");
        break;
    default:
        printf("Unknown ast type");
        break;
    }
}

void print_program(FILE *f, program_t *p) {
    if (p != NULL) {
        print_ast(f, p->ast);
        printf("\n");
        print_program(f, p->next);
    }
}
