#ifndef TREAP_H
#define TREAP_H

// https://cp-algorithms.com/data_structures/treap.html#implementation

#include "garb.h"

#define TR(h) (D((h), treap_node_t *))

typedef struct treap_node {
    long long key, priority;
    handle_t val, l, r;
} treap_node_t;

handle_t treap_insert(handle_t h, long long key, handle_t val);
handle_t treap_search(handle_t h, long long key);
handle_t treap_erase(handle_t h, long long key, handle_t *save_to);
handle_t treap_new(long long key, handle_t val);
void treap_for_each_inorder(handle_t t, void (*f)(void *));

#endif
