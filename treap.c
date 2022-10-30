#include <stdlib.h>
#include <assert.h>

#include "garb.h"
#include "roots.h"
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
    push_root(h);
    handle_t h2 = galloc(sizeof(treap_node_t), rb_trace, NULL);
    push_root(h2);
    *TR(h2) = *TR(h);
    pop_roots(2);
    return h2;
}

handle_t treap_rotate_right(handle_t h) {
    new_frame();
    push_root_in_frame(h);
    treap_node_t *l = TR(TR(h)->l);
    handle_t lr = l->r;
    handle_t nh = treap_copy_shallow(h);
    push_root_in_frame(nh);
    TR(nh)->l = lr;
    handle_t nl = treap_copy_shallow(TR(h)->l);
    push_root_in_frame(nl);
    TR(nl)->r = nh;
    pop_frame();
    return nl;
}

handle_t treap_rotate_left(handle_t h) {
    new_frame();
    push_root_in_frame(h);
    treap_node_t *r = TR(TR(h)->r);
    handle_t rl = r->l;
    handle_t nh = prof(treap_copy_shallow(h));
    TR(nh)->r = rl;
    handle_t nr = prof(treap_copy_shallow(TR(h)->r));
    TR(nr)->l = nh;
    pop_frame();
    return nr;
}

handle_t treap_new(long long key, handle_t val) {
    handle_t h = pro(galloc(sizeof(treap_node_t), rb_trace, NULL));
    assert(h);
    treap_node_t *node = TR(h);
    node->key = key;
    node->val = val;
    node->priority = rand();
    node->l = NULL_HANDLE;
    node->r = NULL_HANDLE;
    pop_root();
    return h;
}

handle_t treap_insert(handle_t t, long long key, handle_t val) {
    if (t == NULL_HANDLE) {
        return treap_new(key, val);
    }
    new_frame();
    t = prof(treap_copy_shallow(t));
    if (key < TR(t)->key) {
        TR(t)->l = prof(treap_insert(TR(t)->l, key, val));
        if (TR(TR(t)->l)->priority > TR(t)->priority) {
            t = prof(treap_rotate_right(t));
        }
    } else if (key != TR(t)->key) {
        TR(t)->r = prof(treap_insert(TR(t)->r, key, val));
        if (TR(TR(t)->r)->priority > TR(t)->priority) {
            t = prof(treap_rotate_left(t));
        }
    }
    pop_frame();
    return t;
}

void treap_for_each_inorder(handle_t t, void (*f)(void *)) {
    if (t == NULL_HANDLE) {
        return;
    }
    push_root(t);
    treap_for_each_inorder(TR(t)->l, f);
    f(TR(t));
    treap_for_each_inorder(TR(t)->r, f);
    pop_root();
}

handle_t treap_search(handle_t t, long long key) {
    if (t == NULL_HANDLE) {
        return NULL_HANDLE;
    }

    pro(t);
    handle_t ans;
    if (key == TR(t)->key) {
        ans = TR(t)->val;
    } else if (key < TR(t)->key) {
        ans = treap_search(TR(t)->l, key);
    } else {
        ans = treap_search(TR(t)->r, key);
    }
    pop_root();
    return ans;
}

// delete key from h, returning whether it was deleted
// makes a copy of h
handle_t treap_erase(handle_t h, long long key, handle_t *save_to) {
    if (h == NULL_HANDLE) {
        return NULL_HANDLE;
    }
    new_frame();
    h = prof(treap_copy_shallow(h));
    if (key == TR(h)->key) {
        if (save_to) *save_to = TR(h)->val;
        if (TR(h)->l == NULL_HANDLE) {
            h = TR(h)->r;
        } else if (TR(h)->r == NULL_HANDLE) {
            h = TR(h)->l;
        } else {
            if (TR(TR(h)->l)->priority > TR(TR(h)->r)->priority) {
                h = prof(treap_rotate_right(h));
                TR(h)->r = prof(treap_erase(TR(h)->r, key, NULL));
            } else {
                h = prof(treap_rotate_left(h));
                TR(h)->l = prof(treap_erase(TR(h)->l, key, NULL));
            }
        }
    } else if (key < TR(h)->key) {
        TR(h)->l = prof(treap_erase(TR(h)->l, key, save_to));
    } else {
        TR(h)->r = prof(treap_erase(TR(h)->r, key, save_to));
    }
    pop_frame();
    return h;
}
