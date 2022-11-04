#include "api.h"

int cmp_mod_apply(cmp_mod_t *mod, handle_t a) {
    return mod->f(mod->e, mod->x, a);
}

void trace_closure(void *p) {
    closure_t *c = p;
    trace(c->sexpr);
    trace(c->args);
    trace(c->env);
}

void finalize_closure(void *p) {
    closure_t *c = p;
    finalize(c->sexpr);
    finalize(c->args);
    finalize(c->env);
}

void assert_is_symbol(handle_t h) {
    if (tag(h) != SYMBOL) {
        printf("Expected a string\n");
        exit(1);
    }
}
