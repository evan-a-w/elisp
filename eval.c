#include "parser.h"
#include "api.h"
#include "const_string.h"
#include "eval.h"
#include "garb.h"
#include "env.h"
#include "map.h"
#include "roots.h"
#include "list.h"
#include "vec.h"

typedef struct {
    handle_t env;
    ul index;
    handle_t l;
} arg_struct_thing;

static eval_state_t st;

void call_on_st_vars(void (*f)(handle_t)) {
    f(st.env);
    for (ul i = 0; i < st.res->size; i++) f(st.res->arr[i]);
    for (ul i = 0; i < st.st->size; i++) f(st.st->arr[i]);
}

void print_list(handle_t h) {
    while (h != NULL_HANDLE) {
        print_value(list_head(h));
        h = list_tail(h);
        if (h != NULL_HANDLE) {
            putchar(' ');
        }
    }
}

void st_init(void) { st = st_new(); }

void st_destroy(void) { st_free(&st); }

bool tail_recurse(void) {
    return roots_size() == 0 || peek_root() == NULL_HANDLE || tag(peek_root()) != SEXPR;
}

void push_arg_into_env(void *data, handle_t h) {
    arg_struct_thing *arg = data;
    arg->env = prof(env_push(arg->env, h, st.res->arr[arg->index++]));
}

handle_t sexpr_to_list(sexpr_t *l) {
    if (l == NULL) return NULL_HANDLE;
    handle_t rest = pro(sexpr_to_list(l->next));
    handle_t res = list_cons(pro(ast_to_val(l->val)), rest);
    pop_roots(2);
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
        case _Fn:
        case _IfE:
        case _LetE:
            f(h);
            break;
        case _VarE:
        case _Apply:
            break;
    }
}

void spec_base_trace(void *p) { spec_base_each(p, trace); }

void spec_base_finalize(void *p) { spec_base_each(p, finalize); }

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
            if (h)
                set_tag(h, SEXPR);
            break;
        case SpecialForm:
        {
            h = pro(sexpr_to_list(ast->special.l));
            set_tag(h, SEXPR);
            handle_t res = galloct(sizeof (spec_base_t), SPEC, spec_base_trace, spec_base_finalize);
            spec_base_t *base = d(res);
            base->data = h;
            base->form = (eval_spec_t)ast->special.spec;
            pop_root();
            h = res;
            break;
        }
        default:
            return NULL_HANDLE;
    }
    return h;
}

void st_free(eval_state_t *st) {
    ul_vfree(st->st);  
    ul_vfree(st->res);  
}

eval_state_t st_new() {
    return (eval_state_t) {
        .st = ul_vinit(10),
        .res = ul_vinit(10),
        .to_eval = 0,
        .env = env_push(NULL_HANDLE, cs_new("ENV"), cs_new("DUMMY"))
    };
}

handle_t st_push_dep(handle_t h) {
    if (st.to_eval) {
        st_rpush(h);
    } else {
        st_push(h);
    }
    return h;
}

handle_t st_rpush(handle_t h) {
    printf("R pushing %lu ", h);
    print_value(h);
    printf("\n");
    ul_push(st.res, h);
    return h;
}

handle_t st_rpop() { return ul_pop(st.res); }

handle_t st_push(handle_t h) {
    printf("pushing %lu ", h);
    print_value(h);
    printf("\n");
    ul_push(st.st, h);
    return h;
}

handle_t st_pop() { return ul_pop(st.st); }

handle_t st_peek() { return ul_peek(st.st); }
handle_t st_rpeek() { return ul_peek(st.res); }

void push_and_eval(void *e, handle_t h) {
    (void)e;
    st_push(h);
    eval();
}

handle_t eval() {
    st.to_eval++;
    do {
        handle_t h = st_peek();
        printf("evaling ");
        print_value(h);
        printf(" of %lu with env ", st.to_eval);
        print_value(st.env);
        printf("\n");
        switch (tag(h)) {
            case ENV:
                st.env = st_pop();
                printf("New env:");
                print_value(st.env);
                printf("\n");
                break;
            case SYMBOL:
                if (!env_search(st.env, h, &h)) {
                    printf("Symbol not found: \"%s\"\n", CS(h)->str);
                    exit(1);
                }
                st.to_eval--;
                st_pop();
                st_push_dep(h);
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
                        handle_t val = list_head(spec->data);
                        if (tag(val) == SEXPR) {
                            set_tag(val, LIST);
                        } else if (tag(val) == SYMBOL) {
                            val = h;
                        }

                        st.to_eval--;
                        st_pop();
                        st_push_dep(val);
                        break;
                    }
                    case _If:
                    {
                        unsigned long len = list_len(spec->data);
                        if (len != 3) {
                            printf("if takes exactly three arguments, got %lu\n", len);
                            exit(1);
                        }
                        handle_t cond = list_head(spec->data);
                        handle_t tl = list_tail(spec->data);
                        handle_t t = list_head(tl);
                        handle_t f = list_head(list_tail(tl));
                        handle_t new_spec = pro(galloct(sizeof(spec_base_t), SPEC, spec_base_trace, spec_base_finalize));
                        spec_base_t *new_spec_d = d(new_spec);
                        new_spec_d->data = hpair_new(t, f);
                        new_spec_d->form = _IfE;
                        pop_root();
                        st.to_eval++;
                        st_pop();
                        st_push(new_spec);
                        st_push(cond);
                        break;
                    }
                    case _IfE:
                    {
                        hpair_t *ife = d(spec->data);
                        handle_t cond = st_rpop();
                        if (tag(cond) != BOOL) {
                            printf("if condition must be a boolean, got %d\n", tag(cond));
                            exit(1);
                        }

                        st_pop();

                        if (B(cond)) {
                            st_push(ife->key);
                        } else {
                            st_push(ife->val);
                        }
                        break;
                    }
                    case _Apply:
                    {
                        ul num_in_sexpr = spec->data;
                        st_pop();
                        ul index_in_res_vec = st.res->size - num_in_sexpr;
                        handle_t f = st.res->arr[index_in_res_vec++];
                        switch (tag(f)) {
                            case CLOSURE:
                            {
                                ul num_args = num_in_sexpr - 1;
                                closure_t *clos = d(f);
                                if (num_args > clos->arity) {
                                    printf("Too many arguments to function, expected %lu, got %lu\n", clos->arity, num_args);
                                    exit(1);
                                } else {
                                    arg_struct_thing a = {
                                        .env = clos->env,
                                        .index = index_in_res_vec,
                                        .l = NULL_HANDLE,
                                    };
                                    new_frame();
                                    handle_t list = clos->args;
                                    ul na = num_args;
                                    while (list && na) {
                                        push_arg_into_env(&a, L(list)->val);
                                        na--;
                                        list = L(list)->next;
                                    }
                                    a.l = list;
                                    pop_frame();
                                    printf("Clos env: ");
                                    print_value(a.env);
                                    printf("\n");
                                    st.res->size -= num_in_sexpr;
                                    if (num_args == D(f, closure_t *)->arity) {
                                        if (!tail_recurse()) {
                                            st_push(st.env);
                                        }
                                        st.env = a.env;
                                        st_push(D(f, closure_t *)->sexpr);
                                    } else {
                                        pro(a.env);
                                        pro(a.l);
                                        handle_t new_clos = galloct(sizeof (closure_t), CLOSURE, trace_closure, finalize_closure);
                                        closure_t *new_clos_d = d(new_clos);
                                        clos = d(f);
                                        pop_roots(2);
                                        *new_clos_d = (closure_t) {
                                            .args = a.l,
                                            .sexpr = clos->sexpr,
                                            .arity = clos->arity - num_args,
                                            .env = a.env,
                                        };
                                        st_push(new_clos);
                                    }
                                }
                                break;
                            }
                            default:
                                printf("Cannot apply non-function\n");
                                exit(1);
                        }
                        break;
                    }
                    case _Do:
                    {
                        st.to_eval += list_len(spec->data) - 1;
                        st_pop();
                        list_for_each_rev(spec->data, (void (*)(handle_t))st_push);
                        break;
                    }
                    case _Fn:
                    {
                        handle_t new = galloct(sizeof (closure_t), CLOSURE, trace_closure, finalize_closure);
                        spec = d(h);
                        if (list_len(spec->data) != 2) {
                            printf("fn takes 2 arguments, got %lu\n", list_len(spec->data));
                            print_value(spec->data);
                            putchar('\n');
                            exit(1);
                        }
                        handle_t args = list_head(spec->data);
                        if (args && tag(args) != SEXPR) {
                            printf("fn's first argument must be an sexpr, got %d\n", tag(args));
                            exit(1);
                        }
                        list_for_each(args, assert_is_symbol);
                        closure_t *new_d = d(new);
                        new_d->args = args;
                        new_d->arity = list_len(args);
                        new_d->sexpr = list_head(list_tail(spec->data));
                        new_d->env = st.env;
                        st_pop();
                        st.to_eval--;
                        st_push_dep(new);
                        break;
                    }
                    case _Var:
                    case _VarE:
                    case _Let:
                    case _LetE:
                        printf("Unimplemented\n");
                        exit(1);
                    default:
                        printf("Unknown form %d\n", spec->form);
                        exit(1);
                }
                break;
            }
            case SEXPR:
            {
                unsigned long len = list_len(h);
                handle_t appl = galloct(sizeof(spec_base_t), SPEC, spec_base_trace, spec_base_finalize);
                spec_base_t *ap = d(appl);
                ap->form = _Apply;
                ap->data = len;
                st_pop();
                st_push(appl);
                list_for_each_rev(h, (void (*)(handle_t))st_push);
                st.to_eval += len;
                break;
            }
            default:
                st.to_eval--;
                st_push_dep(st_pop());
        }
    } while (st.to_eval);
    return st_pop();
}

handle_t eval_ast(ast_t *ast) {
    handle_t val = ast_to_val(ast);
    st_push(val);
    return eval();
}

void print_value_space(handle_t h);

void print_value(handle_t h) {
    pro(h);
    switch (tag(h)) {
        case SPEC:
        {
            spec_base_t *spec = d(h);
            switch (spec->form) {
                case _Quote:
                    printf("'");
                    print_value(spec->data);
                    break;
                case _If:
                    printf("(if ");
                    print_value(spec->data);
                    putchar(' ');
                    print_value(list_head(list_tail(spec->data)));
                    putchar(' ');
                    print_value(list_head(list_tail(list_tail(spec->data))));
                    putchar(')');
                    break;
                case _Do:
                    printf("(do ");
                    print_list(spec->data);
                    putchar(')');
                    break;
                case _Fn:
                    printf("(fn ");
                    print_list(spec->data);
                    putchar(')');
                    break;
                case _Var:
                    printf("(var ");
                    print_value(spec->data);
                    putchar(')');
                    break;
                case _Let:
                    printf("(let ");
                    print_list(spec->data);
                    putchar(')');
                    break;
                default:
                    printf("<spec %d>", spec->form);
            }
            break;
        }
        case INT:
            printf("%lld", I(h));
            break;
        case BOOL:
            printf("%s", B(h) ? "true" : "false");
            break;
        case CLOSURE:
            printf("#<closure(");
            printf("args: ");
            print_value(CL(h)->args);
            printf(", env: ");
            print_value(CL(h)->env);
            printf(", arity: %lu)>\n", CL(h)->arity);
            break;
        case LIST:
            printf("'(");
            print_list(h);
            printf(")");
            break;
        case SEXPR:
            printf("(");
            print_list(h);
            printf(")");
            break;
        case ENV:
        case MAP:
            printf("{");
            map_for_each_inorder(h, print_value_space);
            printf("}");
            break;
        case SYMBOL:
            printf("%s", CS(h)->str);
            break;
        case STR:
            printf("\"%s\"", CS(h)->str);
            break;
        case HPAIR:
            printf("(");
            print_value(HP(h)->key);
            printf(" . ");
            print_value(HP(h)->val);
            printf(")");
            break;
        default:
            printf("#<tag(%d)>", tag(h));
    }
    pop_root();
}

void print_value_space(handle_t h) {
    print_value(h);
    printf(" ");
}
