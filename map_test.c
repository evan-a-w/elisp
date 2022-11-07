#include "map.h"
#include "garb.h"
#include "roots.h"
#include "const_string.h"

int main(void) {
    gc_init();

    new_frame();
    handle_t map = prof(env_push(NULL_HANDLE, prof(cs_new("a")), prof(cs_new("b"))));
    print_value(env);
    printf("\n");
    env = prof(env_push(env, prof(cs_new("z")), prof(cs_new("f"))));
    print_value(env);
    printf("\n");
    env = prof(env_push(env, prof(cs_new("a")), prof(cs_new("c"))));
    print_value(env);
    printf("\n");
    env = prof(env_push(env, prof(cs_new("d")), prof(cs_new("f"))));
    handle_t save;
    print_value(env);
    printf("\n");
    assert(env_search(env, prof(cs_new("a")), &save));
    assert(strcmp(CS(save)->str, "c") == 0);
    env = prof(env_pop(env, prof(cs_new("a"))));
    assert(env_search(env, prof(cs_new("a")), &save));
    assert(strcmp(CS(save)->str, "b") == 0);
    assert(env_search(env, prof(cs_new("d")), &save));
    assert(strcmp(CS(save)->str, "f") == 0);
    pop_frame();

    gc_destroy();
}
