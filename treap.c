#include <stdlib.h>
#include <assert.h>

#include "garb.h"
#include "treap.h"

void rb_trace(void *p) {
    treap_node_t *n = p;
    if (n) {
        trace(n->l);
        trace(n->r);
    }
}

handle_t treap_copy_shallow(handle_t h) {
    if (h == NULL_HANDLE) return NULL_HANDLE;
    handle_t h2 = galloc(sizeof(treap_node_t), rb_trace, NULL);
    *TR(h2) = *TR(h);
    if (rooted(h)) root(h2);
    return h2;
}

handle_t treap_rotate_right(handle_t h) {
    treap_node_t *l = TR(TR(h)->l);
    handle_t lr = l->r;
    handle_t nh = treap_copy_shallow(h);
    TR(nh)->l = lr;
    handle_t nl = treap_copy_shallow(TR(h)->l);
    TR(nl)->r = nh;
    return nl;
}

handle_t treap_rotate_left(handle_t h) {
    treap_node_t *r = TR(TR(h)->r);
    handle_t rl = r->l;
    handle_t nh = treap_copy_shallow(h);
    TR(nh)->r = rl;
    handle_t nr = treap_copy_shallow(TR(h)->r);
    TR(nr)->l = nh;
    return nr;
}

handle_t treap_new_rooted(long long key) {
    handle_t h = galloc_rooted(sizeof(treap_node_t), rb_trace, NULL);
    TR(h)->key = key;
    TR(h)->priority = rand();
    TR(h)->l = NULL_HANDLE;
    TR(h)->r = NULL_HANDLE;
    root(h);
    return h;
}

handle_t treap_new(long long key) {
    handle_t h = galloc(sizeof(treap_node_t), rb_trace, NULL);
    assert(h);
    treap_node_t *node = TR(h);
    node->key = key;
    node->priority = rand();
    node->l = NULL_HANDLE;
    node->r = NULL_HANDLE;
    return h;
}

handle_t treap_insert(handle_t t, long long key) {
    if (t == NULL_HANDLE) {
        return treap_new(key);
    }
    t = treap_copy_shallow(t);
    if (key < TR(t)->key) {
        TR(t)->l = treap_insert(TR(t)->l, key);
        if (TR(TR(t)->l)->priority > TR(t)->priority) {
            t = treap_rotate_right(t);
        }
    } else if (key != TR(t)->key) {
        TR(t)->r = treap_insert(TR(t)->r, key);
        if (TR(TR(t)->r)->priority > TR(t)->priority) {
            t = treap_rotate_left(t);
        }
    }
    return t;
}

void treap_for_each_inorder(handle_t t, void (*f)(void *)) {
    if (t == NULL_HANDLE) {
        return;
    }
    treap_for_each_inorder(TR(t)->l, f);
    f(TR(t));
    treap_for_each_inorder(TR(t)->r, f);
}
