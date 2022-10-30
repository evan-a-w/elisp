#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "treap.h"
#include "roots.h"
#include "garb.h"

int main(void) {
    gc_init();

    handle_t t = pro(treap_new(0, true));
    for (int i = 1; i < 100; i++) {
        pop_root();
        t = pro(treap_insert(t, i, true));
    }

    assert(!treap_search(t, 412));
    for (int i = 1; i < 100; i++) {
        if (!treap_search(t, i)) {
            printf("missing %lld\n", (long long)i);
        }
        assert(treap_search(t, i));
    }

    pop_root();
    t = pro(treap_erase(t, 50, NULL));
    pop_root();
    t = pro(treap_erase(t, 25, NULL));

    assert(!treap_search(t, 412));
    assert(!treap_search(t, 50));
    assert(!treap_search(t, 25));

    for (int i = 1; i < 100; i++) {
        if (i == 25 || i == 50) continue;
        if (!treap_search(t, i)) {
            printf("missing %lld\n", (long long)i);
        }
        assert(treap_search(t, i));
    }

    gc_destroy();
}
