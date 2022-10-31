#ifndef MULTITREAP_H
#define MULTITREAP_H

#include "api.h"
#include "garb.h"
#include "treap.h"

#define M(h) (D((h), map_node_t *))

typedef struct hpair {
    handle_t key;
    handle_t val;
} hpair_t;

typedef struct map_node {
    unsigned long key, priority;
    handle_t hpair_list, l, r;
} map_node_t;

typedef struct {
    handle_t key;
    hash_fn_t key_hash;
    cmp_fn_t key_cmp;
    void *extra;
} map_mod_t;

void hpair_trace(void *p);
void hpair_finalize(void *p);
handle_t hpair_new(handle_t first, handle_t second);
int hpair_cmp_key(void *, handle_t a, handle_t b);

void map_trace(void *p);
void map_finalize(void *p);

handle_t map_insert(handle_t h, map_mod_t *key_pkg, handle_t val, val_update_t f);
bool map_search(handle_t h, map_mod_t *key_pkg, handle_t *save_to);
handle_t map_erase(handle_t h, map_mod_t *key_pkg);
void map_for_each_inorder(handle_t t, void (*f)(handle_t));
handle_t map_erase_if(handle_t h, map_mod_t *key_pkg, bool (*erase)(handle_t));

#endif
