#ifndef STDLIB_H
#define STDLIB_H

#include <string>

#include "garb.h"
#include "env.h"

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

struct string_t {
    size_t len;
    char str[];
};

struct list_node {
    handle_t val;
    handle_t next;
};

typedef Env<std::string, handle_t> env_t;

void init_handlers(void);

// string
handle_t string_new(const char *str);
unsigned long string_hash(handle_t s);
int string_cmp(void *, handle_t a, handle_t b);

// list
void list_finalize(void *l);
void list_trace(void *l);

handle_t list_new(handle_t val);
handle_t list_cons(handle_t a, handle_t l);
handle_t list_head(handle_t l);
handle_t list_tail(handle_t l);
template<typename F>
void list_for_each(handle_t l, F f);
template<typename F>
void list_for_each_rev(handle_t l, F f);
template<typename F>
handle_t list_map(handle_t l, F f);
unsigned long list_len(handle_t l);

// env
void env_finalize(void *l);
void env_trace(void *l);

handle_t env_new(void);
handle_t env_push(handle_t env, std::string &s, handle_t v);
handle_t env_pop(handle_t env, handle_t key);
bool env_lookup(handle_t env, handle_t key, handle_t &val);

#endif
