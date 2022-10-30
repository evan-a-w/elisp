#include "garb.h"
#include "list.h"

void rb_trace(void *p) {
    list_node_t *n = p;
    if (n) {
        trace(n->next);
    }
}

handle_t list_new(handle_t val) {
    handle_t h = galloc(sizeof(list_node_t), rb_trace, NULL);
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
