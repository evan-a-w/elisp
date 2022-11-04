#ifndef STD_H
#define STD_H

#include "garb.h"
#include "parser.h"
#include "eval.h"

#define I(h) (D((h), long long))
#define B(h) (D((h), bool))
#define TAG_EQ(a, b, t) ((a) == NULL_HANDLE || (b) == NULL_HANDLE || ((tag((a)) == tag((b))) && tag((a)) == (t)))

typedef unsigned long (*hash_fn_t)(handle_t);
typedef handle_t (*val_update_t)(handle_t, handle_t);
typedef int (*cmp_fn_t)(void *, handle_t, handle_t);

typedef struct {
    handle_t x;
    cmp_fn_t f;
    void *e;
} cmp_mod_t;

int cmp_mod_apply(cmp_mod_t *mod, handle_t a);

typedef enum {
    NON_USER,
    SPEC,
    SEXPR,
    SYMBOL,
    CFUN,
    CLOSURE,
    INT,
    BOOL,
    STR,
    LIST,
    MAP,
    HPAIR,
    ENV,
} type_tag_t;

typedef enum {
    _If,
    _Var,
    _Let,
    _Quote,
    _Do,
    _Fn,
    _IfE,
    _VarE,
    _LetE,
    _Apply,
} eval_spec_t;

typedef struct {
    eval_spec_t form;
    handle_t data;
} spec_base_t;

typedef handle_t (*cfn_t)(eval_state_t *, size_t);

typedef struct {
    handle_t sexpr;
    handle_t args;
    handle_t env;
    size_t arity;
} closure_t;

void trace_closure(void *p);
void finalize_closure(void *p);

void assert_is_symbol(handle_t);

#endif

