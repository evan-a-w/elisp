#include "map.h"
#include "garb.h"
#include "list.h"
#include "const_string.h"
#include "roots.h"

static map_mod_t map_mod = {
    .key = NULL_HANDLE,
    .key_cmp = cs_cmp,
    .key_hash = cs_hash,
    .extra = NULL
};

static handle_t concat_for_env(handle_t to_insert, handle_t present) {
    pro(to_insert);
    pro(present);
    handle_t in = HP(to_insert)->val;
    handle_t cur = HP(present)->val;
    handle_t res;
    if (tag(cur) == LIST) {
        res = list_cons(in, cur);
    } else {
        res = list_cons(in, pro(list_new(cur)));
        pop_root();
    }
    pro(res);
    handle_t p = hpair_new(HP(to_insert)->key, res);
    pop_roots(3);
    return p;
}

handle_t env_push(handle_t env, handle_t key, handle_t val) {
    pro(env);
    pro(key);
    pro(val);
    map_mod.key = key;
    assert(tag(key) == STR);
    handle_t res = map_insert(env, &map_mod, val, concat_for_env);
    pop_roots(3);
    set_tag(res, ENV);
    return res;
}

handle_t env_pop(handle_t env, handle_t key) {
    new_frame();
    prof(env);
    prof(key);
    map_mod.key = key;
    handle_t save_to;
    bool found;
    handle_t res = prof(map_del(env, &map_mod, &found, &save_to));
    if (found) {
        prof(save_to);
        if (tag(save_to) == LIST) {
            save_to = prof(list_tail(save_to));
            res = env_push(res, key, save_to);
        }
    }
    pop_frame();
    set_tag(res, ENV);
    return res;
}

bool env_search(handle_t env, handle_t key, handle_t *save_to) {
    pro(env);
    pro(key);
    map_mod.key = key;
    bool ans = map_search(env, &map_mod, save_to);
    if (ans && save_to && tag(*save_to) == LIST) *save_to = L(*save_to)->val;
    pop_roots(2);
    return ans;
}
