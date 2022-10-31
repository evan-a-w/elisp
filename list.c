#include "garb.h"
#include "list.h"
#include "roots.h"

void list_trace(void *p) {
    list_node_t *n = p;
    if (n) {
        trace(n->val);
        trace(n->next);
    }
}

handle_t list_copy_shallow(handle_t h) {
    if (h == NULL_HANDLE) return NULL_HANDLE;
    push_root(h);
    handle_t h2 = galloc(sizeof(list_node_t), list_trace, list_finalize);
    *L(h2) = *L(h);
    pop_root();
    return h2;
}

handle_t list_new(handle_t val) {
    handle_t h = galloc(sizeof(list_node_t), list_trace, list_finalize);
    list_node_t *node = L(h);
    node->val = val;
    node->next = NULL_HANDLE;
    return h;
}

handle_t list_cons(handle_t a, handle_t l) {
    handle_t h = list_new(a);
    L(h)->next = l;
    return h;
}

handle_t list_head(handle_t l) {
    return L(l)->val;
}

handle_t list_tail(handle_t l) {
    return L(l)->next;
}

void list_for_each(handle_t l, void (*f)(handle_t)) {
    while (l != NULL_HANDLE) {
        handle_t val = list_head(l);

        f(val);
        l = list_tail(l);
    }
}

void list_finalize(void *p) {
    finalize(((list_node_t *)p)->val);
}

handle_t list_insert_or(handle_t l, cmp_mod_t *cmp, handle_t (*f)(handle_t, handle_t)) {
    if (l == NULL_HANDLE) {
        return list_new(cmp->x);
    }
    handle_t nl = pro(list_copy_shallow(l));
    list_node_t *node = L(nl);
    if (cmp_mod_apply(cmp, node->val) == 0) {
        L(nl)->val = f(cmp->x, L(nl)->val);
    } else {
        L(nl)->next = list_insert_or(L(nl)->next, cmp, f);
    }

    pop_root();
    return nl;
}

bool list_search(handle_t l, cmp_mod_t *cmp, handle_t *save_to) {
    while (l != NULL_HANDLE) {
        handle_t val = list_head(l);
        if (cmp_mod_apply(cmp, val) == 0) {
            if (save_to) *save_to = val;
            return true;
        }
        l = list_tail(l);
    }
    return false;
}
