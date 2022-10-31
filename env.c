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
    handle_t h = list_head(to_insert);
    handle_t res = list_cons(h, present);
    pop_roots(2);
    return res;
}

handle_t env_push(handle_t env, handle_t key, handle_t val) {
    pro(env);
    pro(key);
    pro(val);
    map_mod.key = pro(list_new(val));
    handle_t res = map_insert(env, &map_mod, val, concat_for_env);
    pop_roots(4);
    return res;
}

handle_t env_pop(handle_t env, handle_t key) {
    pro(env);
    pro(key);
    map_mod.key = key;
    handle_t res = map_erase(env, &map_mod);
    pop_roots(2);
    return res;
}

bool env_search(handle_t env, handle_t key, handle_t *save_to) {
    pro(env);
    pro(key);
    map_mod.key = key;
    bool ans = map_search(env, &map_mod, save_to);
    pop_roots(2);
    return ans;
}
