#include "garb.h"
#include "list.h"
#include "roots.h"
#include "api.h"

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
    handle_t h2 = galloct(sizeof(list_node_t), LIST, list_trace, list_finalize);
    *L(h2) = *L(h);
    pop_root();
    return h2;
}

handle_t list_new(handle_t val) {
    handle_t h = galloct(sizeof(list_node_t), LIST, list_trace, list_finalize);
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

void list_for_each_e_rev(handle_t l, void *e, void (*f)(void *, handle_t)) {
    if (l == NULL_HANDLE) return;
    list_for_each_e_rev(L(l)->next, e, f);
    f(e, L(l)->val);
}

void list_for_each_e(handle_t l, void *e, void (*f)(void *, handle_t)) {
    while (l != NULL_HANDLE) {
        f(e, list_head(l));
        l = list_tail(l);
    }
}

void list_for_each(handle_t l, void (*f)(handle_t)) {
    while (l != NULL_HANDLE) {
        f(list_head(l));
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

handle_t list_erase_if(handle_t l, cmp_mod_t *cmp, bool (*erase)(handle_t)) {
    if (l == NULL_HANDLE) return NULL_HANDLE;
    handle_t nl = pro(list_copy_shallow(l));
    list_node_t *node = L(nl);
    if (cmp_mod_apply(cmp, node->val) == 0) {
        pop_root();
        if (erase(node->val)) {
            return node->next;
        } else {
            return l;
        }
    }
    L(nl)->next = list_erase_if(node->next, cmp, erase);
    pop_root();
    return nl;
}

handle_t list_map(handle_t l, void *e, handle_t (*f)(void *, handle_t)) {
    if (l == NULL_HANDLE) return NULL_HANDLE;
    handle_t head = pro(list_copy_shallow(l));
    handle_t nl = head;
    while (nl != NULL_HANDLE) {
        L(nl)->val = f(e, L(nl)->val);
        L(nl)->next = list_copy_shallow(L(nl)->next);
    }
    pop_root();
    return head;
}

unsigned long list_len(handle_t l) {
    unsigned long len = 0;
    while (l != NULL_HANDLE) {
        len++;
        l = list_tail(l);
    }
    return len;
}
