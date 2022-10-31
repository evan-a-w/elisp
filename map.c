#include <stdlib.h>
#include <assert.h>

#include "garb.h"
#include "roots.h"
#include "map.h"
#include "list.h"
#include "api.h"
#include "base_types.h"

void hpair_trace(void *p) {
    hpair_t *hp = p;
    trace(hp->key);
    trace(hp->val);
}

int hpair_cmp_key(void *f, handle_t a, handle_t b) {
    if (!TAG_EQ(a, b, HPAIR)) return -1;
    int (*cmp_fn)(handle_t, handle_t) = f;
    return cmp_fn(D(a, hpair_t *)->key, D(b, hpair_t *)->key);
}

void hpair_finalize(void *p) {
    hpair_t *hp = p;
    finalize(hp->key);
    finalize(hp->val);
}

handle_t hpair_new(handle_t first, handle_t second) {
    handle_t h = galloct(sizeof(hpair_t), HPAIR, hpair_trace, hpair_finalize);
    hpair_t *hp = D(h, hpair_t *);
    hp->key = first;
    hp->val = second;
    return h;
}

void map_trace(void *p) {
    map_node_t *n = p;
    if (n) {
        trace(n->l);
        trace(n->hpair_list);
        trace(n->r);
    }
}

void map_finalize(void *p) {
    map_node_t *n = p;
    if (n) {
        finalize(n->l);
        finalize(n->hpair_list);
        finalize(n->r);
    }
}

handle_t map_copy_shallow(handle_t h) {
    if (h == NULL_HANDLE) return NULL_HANDLE;
    push_root(h);
    handle_t h2 = galloct(sizeof(map_node_t), MAP, map_trace, map_finalize);
    *M(h2) = *M(h);
    pop_root();
    return h2;
}

handle_t map_rotate_right(handle_t h) {
    new_frame();
    push_root_in_frame(h);
    map_node_t *l = M(M(h)->l);
    handle_t lr = l->r;
    handle_t nh = map_copy_shallow(h);
    push_root_in_frame(nh);
    M(nh)->l = lr;
    handle_t nl = map_copy_shallow(M(h)->l);
    M(nl)->r = nh;
    pop_frame();
    return nl;
}

handle_t map_rotate_left(handle_t h) {
    new_frame();
    push_root_in_frame(h);
    map_node_t *r = M(M(h)->r);
    handle_t rl = r->l;
    handle_t nh = prof(map_copy_shallow(h));
    M(nh)->r = rl;
    handle_t nr = prof(map_copy_shallow(M(h)->r));
    M(nr)->l = nh;
    pop_frame();
    return nr;
}

handle_t map_insert_(handle_t t, unsigned long key, map_mod_t *key_pkg, handle_t val, val_update_t f) {
    if (t == NULL_HANDLE) {
        handle_t res = galloct(sizeof(map_node_t), MAP, map_trace, map_finalize);
        pro(res);
        handle_t hp = hpair_new(key_pkg->key, val);
        pro(hp);
        *M(res) = (map_node_t) {
            .key = key_pkg->key_hash(key_pkg->key),
            .priority = rand(),
            .l = NULL_HANDLE,
            .hpair_list = list_new(hp),
            .r = NULL_HANDLE,
        };
        pop_roots(2);
        return res;
    }
    new_frame();
    t = prof(map_copy_shallow(t));
    if (key < M(t)->key) {
        M(t)->l = prof(map_insert(M(t)->l, key_pkg, val, f));
        if (M(M(t)->l)->priority > M(t)->priority) {
            t = prof(map_rotate_right(t));
        }
    } else if (key != M(t)->key) {
        M(t)->r = prof(map_insert(M(t)->r, key_pkg, val, f));
        if (M(M(t)->r)->priority > M(t)->priority) {
            t = prof(map_rotate_left(t));
        }
    } else {
        handle_t to_insert = prof(hpair_new(key_pkg->key, val));
        cmp_mod_t cmp_mod = (cmp_mod_t) {
            .x = to_insert,
            .f = hpair_cmp_key,
            .e = key_pkg->key_cmp,
        };
        M(t)->hpair_list = 
            prof(list_insert_or(M(t)->hpair_list,
                                &cmp_mod, f));
    }
    pop_frame();
    return t;
}

handle_t map_insert(handle_t t, map_mod_t *key_pkg, handle_t val, val_update_t f) {
    long long key = key_pkg->key_hash(key_pkg->key);
    return map_insert_(t, key, key_pkg, val, f);
}

void map_for_each_inorder(handle_t t, void (*f)(handle_t)) {
    if (t == NULL_HANDLE) {
        return;
    }
    push_root(t);
    map_for_each_inorder(M(t)->l, f);
    list_for_each(M(t)->hpair_list, f);
    map_for_each_inorder(M(t)->r, f);
    pop_root();
}

bool map_search_(handle_t t, unsigned long key, map_mod_t *key_pkg, handle_t *save_to) {
    if (t == NULL_HANDLE) {
        return NULL_HANDLE;
    }

    new_frame();
    prof(t);
    bool ans = false;
    if (key == M(t)->key) {
        handle_t to_search = prof(hpair_new(key_pkg->key, NULL_HANDLE));
        cmp_mod_t cmp_mod = (cmp_mod_t) {
            .x = to_search,
            .f = hpair_cmp_key,
            .e = key_pkg->key_cmp,
        };
        ans = list_search(M(t)->hpair_list, &cmp_mod, save_to);
    } else if (key < M(t)->key) {
        ans = map_search(M(t)->l, key_pkg, save_to);
    } else {
        ans = map_search(M(t)->r, key_pkg, save_to);
    }
    pop_frame();
    return ans;
}

bool map_search(handle_t t, map_mod_t *key_pkg, handle_t *save_to) {
    unsigned long key = key_pkg->key_hash(key_pkg->key);
    return map_search_(t, key, key_pkg, save_to);
}

handle_t map_erase_(handle_t h, unsigned long key, map_mod_t *key_pkg) {
    if (h == NULL_HANDLE) {
        return NULL_HANDLE;
    }
    new_frame();
    h = prof(map_copy_shallow(h));
    if (key == M(h)->key) {
        if (M(h)->l == NULL_HANDLE) {
            h = M(h)->r;
        } else if (M(h)->r == NULL_HANDLE) {
            h = M(h)->l;
        } else {
            if (M(M(h)->l)->priority > M(M(h)->r)->priority) {
                h = prof(map_rotate_right(h));
                M(h)->r = prof(map_erase_(M(h)->r, key, key_pkg));
            } else {
                h = prof(map_rotate_left(h));
                M(h)->l = prof(map_erase_(M(h)->l, key, key_pkg));
            }
        }
    } else if (key < M(h)->key) {
        M(h)->l = prof(map_erase_(M(h)->l, key, key_pkg));
    } else {
        M(h)->r = prof(map_erase_(M(h)->r, key, key_pkg));
    }
    pop_frame();
    return h;
}

handle_t map_erase_if_(handle_t h, unsigned long key, map_mod_t *key_pkg,
                       bool (*erase)(handle_t)) {
    if (h == NULL_HANDLE) {
        return NULL_HANDLE;
    }
    new_frame();
    h = prof(map_copy_shallow(h));
    if (key == M(h)->key) {
        handle_t to_erase = prof(hpair_new(key_pkg->key, NULL_HANDLE));
        cmp_mod_t cmp_mod = (cmp_mod_t) {
            .x = to_erase,
            .f = hpair_cmp_key,
            .e = key_pkg->key_cmp,
        };
        M(h)->hpair_list = prof(list_erase_if(M(h)->hpair_list, &cmp_mod, erase));
        if (M(h)->l == NULL_HANDLE) {
            h = M(h)->r;
        } else if (M(h)->r == NULL_HANDLE) {
            h = M(h)->l;
        } else {
            if (M(M(h)->l)->priority > M(M(h)->r)->priority) {
                h = prof(map_rotate_right(h));
                M(h)->r = prof(map_erase_(M(h)->r, key, key_pkg));
            } else {
                h = prof(map_rotate_left(h));
                M(h)->l = prof(map_erase_(M(h)->l, key, key_pkg));
            }
        }
    } else if (key < M(h)->key) {
        M(h)->l = prof(map_erase_(M(h)->l, key, key_pkg));
    } else {
        M(h)->r = prof(map_erase_(M(h)->r, key, key_pkg));
    }
    pop_frame();
    return h;
}

handle_t map_erase(handle_t h, map_mod_t *key_pkg) {
    unsigned long key = key_pkg->key_hash(key_pkg->key);
    return map_erase_(h, key, key_pkg);
}

handle_t map_erase_if(handle_t h, map_mod_t *key_pkg, bool (*erase)(handle_t)) {
    unsigned long key = key_pkg->key_hash(key_pkg->key);
    return map_erase_if_(h, key, key_pkg, erase);
}
