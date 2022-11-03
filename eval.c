#include "parser.h"
#include "api.h"
#include "const_string.h"
#include "eval.h"
#include "garb.h"
#include "env.h"
#include "roots.h"
#include "list.h"
#include "vec.h"

bool tail_recurse(eval_state_t *st) {
    return roots_size() == 0 || peek_root() == NULL_HANDLE || tag(peek_root()) != SEXPR;
}

handle_t sexpr_to_list(sexpr_t *l) {
    if (l == NULL) return NULL_HANDLE;
    handle_t rest = pro(sexpr_to_list(l->next));
    handle_t res = list_cons(ast_to_val(l->val), rest);
    pop_root();
    return res;
}

void spec_base_each(spec_base_t *sb, void (*f)(handle_t)) {
    eval_spec_t form = sb->form;
    handle_t h = sb->data;
    switch (form) {
    case _If:
    case _Var:
    case _Let:
    case _Quote:
    case _Do:
        f(h);
        break;
    case _IfE:
        f(D(h, ife_t *)->t);
        f(D(h, ife_t *)->f);
        break;
    case _VarE:
    case _Apply:
        break;
    case _LetE:
        f(D(h, lete_t *)->list_to_set);
        f(D(h, lete_t *)->body);
        break;
    }
}

void spec_base_trace(void *p) {
    spec_base_each(p, trace);
}

void spec_base_finalize(void *p) {
    spec_base_each(p, finalize);
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
            set_tag(h, SEXPR);
            break;
        case SpecialForm:
        {
            h = pro(sexpr_to_list(ast->special.l));
            set_tag(h, SEXPR);
            handle_t res = galloct(sizeof (spec_base_t), SPEC, spec_base_trace, spec_base_finalize);
            spec_base_t *base = (spec_base_t *)d(res);
            base->data = h;
            base->form = (eval_spec_t)ast->special.spec;
            pop_root();
            h = res;
            break;
        }
        default:
            free_ast(ast);
            return NULL_HANDLE;
    }
    free_ast(ast);
    return h;
}

void st_free(eval_state_t *st) {
    ul_vfree(st->st);  
}

eval_state_t st_new() {
    return (eval_state_t) {
        .st = NULL,
        .env = NULL_HANDLE,
    };
}

handle_t st_push(eval_state_t *st, handle_t h) {
    ul_push(st->st, h);
    return h;
}

handle_t st_pop(eval_state_t *st) {
    return ul_pop(st->st);
}

handle_t st_peek(eval_state_t *st) {
    return ul_peek(st->st);
}

void push_and_eval(void *e, handle_t h) {
    eval_state_t *st = e;
    st_push(st, pro(h));
    eval(st);
}

handle_t eval(eval_state_t *st) {
    ul to_eval = 1;
    do {
        handle_t h = st_pop(st);
        switch (tag(h)) {
        case NON_USER:
            pop_root();
            unroot_global(st->env);
            st->env = root_global(h);
            break;
        case INT:
        case BOOL:
        case STR:
        case LIST:
        case CLOSURE:
            to_eval--;
            st_push(st, h);
            break;
        case SYMBOL:
            if (!env_search(st->env, h, &h)) {
                printf("Symbol not found: \"%s\"\n", CS(h)->str);
                exit(1);
            }
            to_eval--;
            st_push(st, h);
            break;
        case SPEC:
        {
            spec_base_t *spec = d(h);
            switch (spec->form) {
            case _Quote:
            {
                unsigned long len = list_len(spec->data);
                if (len != 1) {
                    printf("quote takes exactly one argument, got %lu\n", len);
                    exit(1);
                }
                to_eval--;
                handle_t val = list_head(spec->data);
                if (tag(val) == SEXPR) {
                    set_tag(val, LIST);
                } else if (tag(val) == SYMBOL) {
                    val = h;
                }

                st_push(st, pro(val));
                break;
            }
            case _If:
            {
                to_eval++;
                unsigned long len = list_len(spec->data);
                if (len != 3) {
                    printf("if takes exactly three arguments, got %lu\n", len);
                    exit(1);
                }
                handle_t cond = list_head(spec->data);
                handle_t tl = list_tail(spec->data);
                handle_t t = list_head(tl);
                handle_t f = list_head(list_tail(tl));
                spec->data = galloct(sizeof (ife_t), NON_USER, NULL, NULL);
                spec->form = _IfE;
                D(spec->data, ife_t *)->f = f;
                D(spec->data, ife_t *)->t = t;
                st_push(st, pro(cond));
                handle_t 
            }
            case _IfE:
            {
                
            }
            }
        }
        case SEXPR:
        {
            unsigned long len = list_len(h);
            break;
        }
        default:
            fprintf(stderr, "Unknown tag: %d\n", tag(h));
            assert(false);
        }
    } while (to_eval);
    return pop_root(), st_pop(st);
}

handle_t eval_ast(eval_state_t *st, ast_t *ast) {
    st_push(st, pro(ast_to_val(ast)));
    handle_t res = eval(st);
    pop_root();
    return res;
}
