#include "parser.h"
#include "base_types.h"
#include "const_string.h"
#include "eval.h"
#include "garb.h"
#include "env.h"
#include "roots.h"

bool tail_recurse(eval_state_t *st) {
    return st->stack_sz == 0 || st->stack[st->stack_sz - 1] == NULL_HANDLE
           || tag(st->stack[st->stack_sz - 1]) != SEXPR;
}

handle_t sexpr_to_list(sexpr_t *l) {
    if (l == NULL) return NULL_HANDLE;
    handle_t rest = pro(sexpr_to_list(l->next));
    handle_t res = list_cons(ast_to_val(l->val), rest);
    pop_root();
    return res;
}

handle_t ast_to_val(ast_t *ast) {
    handle_t h;
    switch (ast->type) {
        case AtomInt:
            h = galloct(0, INT, NULL, NULL);
            unsafe_set_data(h, (void *)ast->i);
            break;
        case AtomString:
            h = cs_new(ast->s);
            break;
        case AtomSymbol:
            h = cs_new(ast->s);
            set_tag(h, SYMBOL);
            break;
        case AtomBool:
            h = galloct(0, BOOL, NULL, NULL);
            unsafe_set_data(h, (void *)ast->b);
            break;
        case SExpr:
            h = sexpr_to_list(ast->l);
            break;
        case SpecialForm:
            h = sexpr_to_list(ast->l);
            set_tag(h, SPEC);
            break;
        default:
            free_ast(ast);
            return NULL_HANDLE;
    }
    free_ast(ast);
    return h;
}

handle_t eval_ast(eval_state_t *st, handle_t ast) {
    handle_t res;
    switch (ast->type) {
        case AtomInt:
        {
            res = galloct(0, INT, NULL, NULL);
            unsafe_set_data(res, (void *)ast->i);
            break;
        }
        case AtomString:
            res = cs_new(ast->s);
            break;
        case AtomSymbol:
        {
            if (!env_search(st->env, pro(cs_new(ast->s)), &res))
                printf("Symbol not found: %s\n", ast->s);
            pop_root();

        }
        case SExpr:
        default:
            printf("Unexpected expression: ");
            print_ast(stdout, ast); // HERE
            printf("\n");
            return NULL_HANDLE;
    }
    free_ast(ast);
    return res;
}
