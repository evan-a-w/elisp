#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "treap.h"
#include "garb.h"

typedef struct test_node {
    handle_t val;
    long long hash;
    struct test_node *next;
} sn;

unsigned long hashli(unsigned long x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;

    x = x ^ (x >> 31);
    return x;
}

long long val = 0;

void f(void *p) {
    treap_node_t *n = p;
    val = hashli(val + n->key);
    val = hashli(val + n->priority);
}

int main(void) {
    gc_init();

    handle_t t = treap_new_rooted(0);
    sn *l = malloc(sizeof *l);
    l->val = t;
    l->next = NULL;
    treap_for_each_inorder(t, f);
    l->hash = val;
    val = 0;

    for (int i = 1; i < 100; i++) {
        val = 0;
        treap_for_each_inorder(t, f);
        sn *h = malloc(sizeof(sn));
        h->next = l;
        h->val = t;
        h->hash = val;
        l = h;

        for (int j = 0; j < 100; j++) {
            for (sn *curr = l; curr; curr = curr->next) {
                val = 0;
                treap_for_each_inorder(curr->val, f);
                if (curr->hash != val) {
                    printf("hash mismatch (%lld != %lld) with key %lld\n", curr->hash, val, TR(curr->val)->key);
                }
                assert(curr->hash == val);
            }
        }

        t = treap_insert(t, i);
    }

    gc_destroy();
}
