#include <string.h>

#include "garb.h"
#include "env.h"
#include "roots.h"
#include "stdlib.h"

void init_handlers(void) {
    set_trace(LIST, list_trace);
    set_finalize(LIST, list_finalize);
    set_trace(ENV, env_trace);
    set_trace(ENV, env_finalize);
}

handle_t cs_new(const char *str) {
    size_t len = strlen(str);
    handle_t h = galloc(sizeof(string_t) + len + 1, STR);
    string_t &cs = d<string_t>(h);
    cs.len = len;
    memcpy(cs.str, str, len + 1);
    return h;
}

// from https://stackoverflow.com/questions/7666509/hash-function-for-string
unsigned long cs_hash(handle_t h) {
    const char *str = d<string_t>(h).str;
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int cs_cmp(void *ignore, handle_t a, handle_t b) {
    (void)ignore;
    if ((tag(a) != STR && tag(a) != SYMBOL) || (tag(b) != STR && tag(b) != SYMBOL)) {
        printf("Warning: comparing handles of different types (expected STR, got %d and %d)\n", tag(a), tag(b));
    }
    return strcmp(d<string_t>(a).str, d<string_t>(b).str);
}

handle_t list_new(handle_t val) {
    handle_t h = galloc(sizeof(list_node), LIST);
    list_node &ln = d<list_node>(h);
    ln.val = val;
    ln.next = NULL_HANDLE;
    return h;
}

handle_t list_cons(handle_t a, handle_t l) {
    handle_t h = galloc(sizeof(list_node), LIST);
    list_node &ln = d<list_node>(h);
    ln.val = a;
    ln.next = l;
    return h;
}

handle_t list_head(handle_t l) {
    return d<list_node>(l).val;
}

handle_t list_tail(handle_t l) {
    return d<list_node>(l).next;
}

template<typename F>
void list_for_each_(handle_t l, F f) {
    while (l != NULL_HANDLE) {
        f(list_head(l));
        l = list_tail(l);
    }
}

template<typename F>
void list_for_each_rev_(handle_t l, F f) {
    if (l == NULL_HANDLE) return;
    list_for_each_rev(list_tail(l), f);
    f(list_head(l));
}

template<typename F>
handle_t list_map_(handle_t l, F f) {
    if (l == NULL_HANDLE) return NULL_HANDLE;
    return prof(list_cons(prof(f(list_head(l))), prof(list_map(list_tail(l), f))));
}

template<typename F>
void list_for_each_rev(handle_t l, F f) {
    new_frame();
    prof(l);
    list_for_each_rev_(l, f);
    pop_frame();
}

template<typename F>
void list_for_each(handle_t l, F f) {
    new_frame();
    prof(l);
    list_for_each(l, f);
    pop_frame();
}

template<typename F>
handle_t list_map(handle_t l, F f) {
    new_frame();
    prof(l);
    handle_t ret = list_map_(l, f);
    pop_frame();
    return ret;
}

unsigned long list_len(handle_t l) {
    unsigned long len = 0;
    list_for_each(l, [&len](handle_t) { len++; });
    return len;
}

void env_finalize(void *l) {
    env_t *env = (env_t *)l;
    env->for_each_value(finalize);
    env->~env_t();
}

void env_trace(void *l) {
    env_t *env = (env_t *)l;
    env->for_each_value(trace);
}

handle_t env_new(void) {
    handle_t h = galloc(sizeof(env_t), ENV);
    env_t &e = d<env_t>(h);
    e.root = nullptr;
    return h;
}

handle_t env_push(handle_t env, std::string &s, handle_t v) {
    push_root(env);
    env_t &a = d<env_t>(env);
    handle_t res = galloc(sizeof(env_t), ENV);
    env_t *b = dp<env_t>(env);
    *b = a.push(s, v);
    pop_root();
    return res;
}

handle_t env_pop(handle_t env, std::string &key) {
    push_root(env);
    env_t &a = d<env_t>(env);
    handle_t res = galloc(sizeof(env_t), ENV);
    env_t *b = dp<env_t>(env);
    *b = a.pop(key);
    pop_root();
    return res;
}

bool env_lookup(handle_t env, std::string &key, handle_t &val) {
    env_t &e = d<env_t>(env);
    return e.search(key, val);
}
