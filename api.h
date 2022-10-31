#ifndef STD_H
#define STD_H

#include "garb.h"

typedef unsigned long (*hash_fn_t)(handle_t);
typedef handle_t (*val_update_t)(handle_t, handle_t);
typedef int (*cmp_fn_t)(void *, handle_t, handle_t);

typedef struct {
    handle_t x;
    cmp_fn_t f;
    void *e;
} cmp_mod_t;

int cmp_mod_apply(cmp_mod_t *mod, handle_t a) {
    return mod->f(mod->e, mod->x, a);
}

#endif
