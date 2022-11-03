#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tokeniser.h"
#include "parser.h"

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
        free(l);
    }
}

void free_program(program_t *l) {
    if (l != NULL) {
        free_ast(l->ast);
        free_program(l->next);
        free(l);
    }
}

void free_ast(ast_t *ast) {
    switch (ast->type) {
    case AstError:
    case AtomString:
    case AtomSymbol:
        free(ast->s);
        break;
    case SExpr:
        free_sexpr(ast->l);
        break;
    default:
        break;
    }
    free(ast);
}

ast_t *process_special(ast_t *sexpr) {
    assert(sexpr->type == SExpr);
    sexpr_t *l = sexpr->l;
    if (l != NULL) {
        ast_t *first = l->val;
        if (first->type == AtomSymbol) {
            special_form_t spec;
            bool is_spec = false;
            if ((is_spec = (strcmp(first->s, "quote") == 0))) {
                spec = Quote;
            } else if ((is_spec = (strcmp(first->s, "if") == 0))) {
                spec = If;
            } else if ((is_spec = (strcmp(first->s, "var") == 0))) {
                spec = Var;
            } else if ((is_spec = (strcmp(first->s, "let") == 0))) {
                spec = Let;
            } else if ((is_spec = (strcmp(first->s, "do") == 0))) {
                spec = Do;
            }
            if (is_spec) {
                free_ast(first);
                sexpr_t *n = l->next;
                free(l);
                sexpr->l = n;
                sexpr->type = SpecialForm;
                sexpr->special = spec;
            }
        }
    } else {
        free_ast(sexpr);
        sexpr = malloc(sizeof *sexpr);
        sexpr->type = AstError;
        sexpr->s = allocate_string("Empty S-Expression");
    }
    return sexpr;
}

ast_t *parse_inside_sexpr(tokeniser_t *t) {
    ast_t *res = malloc(sizeof *res);
    sexpr_t *s = NULL;
    *res = (ast_t) { .type = SExpr, .l = s };
    for (;;) {
        token_t curr_token = peek_token(t);
        if (error_token(&curr_token)) {
            free_ast(res);
            return tok_to_error_ast(&curr_token);
        } else if (curr_token.tok == RParen) {
            next_token(t);
            return process_special(res);
        }

        ast_t *next = parse_one(t);
        if (next->type == AstError) {
            free_ast(res);
            return next;
        }

        if (s == NULL) {
            s = malloc(sizeof *s);
            *s = (sexpr_t) { .val = next, .next = NULL };
            res->l = s;
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
        return make_ast((ast_t) { .type = AtomSymbol, .s = curr_token.s });
    case Str:
        if (strcmp(curr_token.s, "T") == 0) {
            free(curr_token.s);
            return make_ast((ast_t) { .type = AtomBool, .b = true });
        } else if (strcmp(curr_token.s, "F") == 0) {
            free(curr_token.s);
            return make_ast((ast_t) { .type = AtomBool, .b = false });
        }
        return make_ast((ast_t) { .type = AtomString, .s = curr_token.s });
    case Int:
        return make_ast((ast_t) { .type = AtomInt, .i = curr_token.i });
    case QuoteMark:
    {
        ast_t *next = parse_one(t);
        ast_t *res = make_ast((ast_t) { .type = SExpr, .l = NULL });
        res->l = malloc(sizeof *res->l);
        res->l->val = make_ast((ast_t) { .type = SpecialForm, .special = Quote, });
        res->l->next = malloc(sizeof *res->l->next);
        *res->l->next = (sexpr_t) { .val = next, .next = NULL };
        return res;
    }
    default:
    {
        char *token = token_to_string(&curr_token);
        free_token(curr_token);
        char *fst = "Unexpected token: ";
        char *res = malloc(strlen(fst) + strlen(token) + 1);
        strcpy(res, fst);
        strcat(res, token);
        free(token);
        return make_ast((ast_t) { .type = AstError, .s = res });
    }
    }
}

program_t *parse(tokeniser_t *t) {
    program_t *res = NULL, *curr = NULL;
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
            res = curr = new_p;
        } else {
            curr->next = new_p;
            curr = new_p;
        }
    }
    return res;
}

void print_ast(FILE *f, ast_t *ast) {
    switch (ast->type) {
    case AstError:
        printf("Error: %s", ast->s);
        break;
    case AtomBool:
        fprintf(f, "%s", ast->b ? "T" : "F");
        break;
    case AtomString:
        printf("\"%s\"", ast->s);
        break;
    case AtomSymbol:
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
    case SpecialForm:
    {
        char *s;
        switch (ast->special) {
        case Quote:
            s = "quote";
        case If:
            s = "if";
            break;
        case Var:
            s = "var";
            break;
        case Let:
            s = "let";
            break;
        case Do:
            s = "do";
            break;
        }
        printf("(%s", s);
        for (sexpr_t *s = ast->l; s != NULL; s = s->next) {
            printf(" ");
            print_ast(f, s->val);
        }
        printf(")");
        break;
    }
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

void debug_print_ast(ast_t *ast) {
    fprintf(stderr, "AST: { type: %d }", ast->type);
    switch (ast->type) {
    case AstError:
        fprintf(stderr, ", { s: %s }", ast->s);
        break;
    case AtomBool:
        fprintf(stderr, ", { b: %d }", ast->b);
        break;
    case AtomString:
        fprintf(stderr, ", { s: %s }", ast->s);
        break;
    case AtomSymbol:
        fprintf(stderr, ", { s: %s }", ast->s);
        break;
    case AtomInt:
        fprintf(stderr, ", { i: %lld }", ast->i);
        break;
    case SExpr:
        fprintf(stderr, ", { l: %p }", (void *)ast->l);
        break;
    default:
        fprintf(stderr, ", { unknown }");
        break;
    }
}
