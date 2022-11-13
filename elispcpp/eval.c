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
            handle_t t = h;
            while (t) {
                set_tag(t, SEXPR);
                t = list_tail(t);
            }
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
        .env = env_push(NULL_HANDLE, cs_new("ENV"), cs_new("DUMMY")),
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
    // printf("R pushing %lu ", h);
    // print_value(h);
    // printf("\n");
    ul_push(st.res, h);
    return h;
}

handle_t st_rpop() { return ul_pop(st.res); }

handle_t st_push(handle_t h) {
    // printf("pushing %lu ", h);
    // print_value(h);
    // printf("\n");
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

void do_spec_quote(handle_t h, spec_base_t *spec) {
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
}

void do_spec_if(handle_t h, spec_base_t *spec) {
    (void)h;
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
}

void do_spec_ife(handle_t h, spec_base_t *spec) {
    (void)h;
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
}

void do_spec_apply(handle_t h, spec_base_t *spec) {
    (void)h;
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
        printf("Cannot apply non-function ");
        print_value(f);
        printf("\n");
        exit(1);
    }
}

void do_spec_fn(handle_t h, spec_base_t *spec) {
    handle_t new = galloct(sizeof (closure_t), CLOSURE, trace_closure,
                           finalize_closure);
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
}

void transform_var_decl(handle_t name, handle_t *def) {
    if (tag(name) == SEXPR) {
        handle_t ndef = prof(galloct(sizeof(spec_base_t), SPEC, spec_base_trace, spec_base_finalize));
        *D(ndef, spec_base_t *) = (spec_base_t) {
            .form = _Fn,
            .data = prof(list_cons(list_tail(name), prof(list_new(*def)))),
        };
        *def = ndef;
    }
}

void do_spec_var(handle_t h, spec_base_t *spec) {
    new_frame();
    handle_t l = prof(spec->data);
    if (list_len(l) != 3) {
        printf("var takes 3 arguments, got %lu\n", list_len(l));
        exit(1);
    }
    handle_t name = prof(list_head(l));
    handle_t rest = list_tail(l);
    handle_t def = prof(list_head(rest));
    transform_var_decl(name, &def);
    handle_t body = prof(list_head(list_tail(rest)));
    prof(h);
    st_pop();
    st_push(st.env); 
    st_push(body);
    *D(h, spec_base_t *) = (spec_base_t) {
        .form = _VarE,
        .data = name,
    };
    st_push(h);
    st_push(def);
    pop_frame();
    st.to_eval += 2;
}

void do_spec_vare(handle_t h, spec_base_t *spec) {
    (void)h;
    handle_t name = spec->data;
    handle_t def = pro(st_rpop());
    bool fn = tag(name) == SEXPR;
    if (fn) name = list_head(name);
    if (tag(name) != SYMBOL) {
        printf("var's first argument must be a symbol, got %d\n", tag(name));
        exit(1);
    }
    st.env = env_push(st.env, name, def);
    pop_root();
    st.to_eval--;
    st_pop();
}

void do_spec_let(handle_t h, spec_base_t *spec) {
    new_frame();

    if (list_len(spec->data) != 2) {
        printf("let takes 2 arguments, got %lu\n", list_len(spec->data));
        exit(1);
    }
    
    st_pop();
    handle_t l = prof(spec->data);
    handle_t bindings = list_head(l);
    st_push(list_head(list_tail(spec->data)));
    st_push(h);
    *D(h, spec_base_t *) = (spec_base_t) {
        .form = _LetE,
        .data = bindings,
    };
    if (tag(bindings) != SEXPR) {
        printf("let's first argument must be an sexpr, got %d\n", tag(bindings));
        exit(1);
    }
    ul bindings_len = list_len(bindings);
    if (bindings_len % 2) {
        printf("let's first argument must have an even number of elements\n");
        exit(1);
    }
    st.to_eval += bindings_len / 2 + 1;
    for (handle_t n = bindings; n; n = L(n)->next) {
        handle_t name = list_head(n);
        handle_t def = list_head(n = L(n)->next);
        transform_var_decl(name, &def);
        st_push(def);
    } 

    pop_frame();
}

handle_t is_merge_env(handle_t key, handle_t val) {
    if (tag(key) == SEXPR && tag(val) == CLOSURE)
        return list_head(key);
    return NULL_HANDLE;
}

void do_spec_lete(handle_t h, spec_base_t *spec) {
    new_frame();
    prof(h);
    handle_t bindings = spec->data;
    handle_t bl = NULL_HANDLE, new_list = NULL_HANDLE;
    for (handle_t l = bindings; l; l = L(l)->next) {
        handle_t name = list_head(l);
        l = L(l)->next;
        handle_t evaled = prof(st_rpop());
        if (tag(name) == SEXPR) {
            name = list_head(name);
            if (tag(evaled) == CLOSURE) {
                handle_t new = prof(list_new(prof(hpair_new(name, evaled))));
                if (new_list == NULL_HANDLE)
                    bl = new_list = new;
                else
                    new_list = L(new_list)->next = new;
            }
        }
        if (tag(name) != SYMBOL) {
            printf("let's first argument must be an sexpr of symbols, got %d\n", tag(name));
            exit(1);
        }
        st.env = env_push(st.env, name, evaled);
    }

    for (handle_t l = bl; l; l = L(l)->next) {
        handle_t clos = prof(HP(L(l)->val)->val);
        for (handle_t l2 = bl; l2; l2 = L(l2)->next) {
            handle_t name2 = HP(L(l2)->val)->key;
            handle_t val = HP(L(l2)->val)->val;
            handle_t clenv = D(clos, closure_t *)->env;
            D(clos, closure_t *)->env = env_push(clenv, name2, val);
        }
    }
    st.to_eval--;
    st_pop();
    pop_frame();
}

void do_spec() {
    handle_t h = st_peek();
    spec_base_t *spec = d(h);
    switch (D(h, spec_base_t *)->form) {
    case _Quote:
        do_spec_quote(h, spec);
        break;
    case _If:
        do_spec_if(h, spec);
        break;
    case _IfE:
        do_spec_ife(h, spec);
        break;
    case _Apply:
        do_spec_apply(h, spec);
        break;
    case _Do:
        st.to_eval += list_len(spec->data) - 1;
        st_pop();
        list_for_each_rev(spec->data, (void (*)(handle_t))st_push);
        break;
    case _Fn:
        do_spec_fn(h, spec);
        break;
    case _Var:
        do_spec_var(h, spec);
        break;
    case _VarE:
        do_spec_vare(h, spec);
        break;
    case _Let:
        do_spec_let(h, spec);
        break;
    case _LetE:
        do_spec_lete(h, spec);
        break;
    default:
        printf("Unknown form %d\n", spec->form);
        exit(1);
    }
}

handle_t eval() {
    st.to_eval++;
    do {
        handle_t h = st_peek();
        printf("evaling ");
        print_value(h);
        printf(" of %lu\n", st.to_eval);
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
            do_spec();
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
            printf("#<env>");
            break;
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
