#include <stdlib.h>
#include <assert.h>

#include "garb.h"
#include "long_table.h"

typedef struct node {
    long long val;
    struct node *prev;
    struct node *next;
} node_t;

static long_table_t roots = NULL;
static node_t *root_list = NULL;

void for_each_root(void (*f)(handle_t)) {
    for (node_t *node = root_list; node != NULL; node = node->next) {
        f(node->val);
    }
}

void init_roots(void) {
    roots = long_table_new();
    root_list = NULL;
}

void destroy_roots(void) {
    long_table_free(roots);
    while (root_list) {
        node_t *next = root_list->next;
        free(root_list);
        root_list = next;
    }
}

void root(handle_t handle) {
    node_t *n = malloc(sizeof *n);
    n->val = handle;
    n->next = root_list;
    if (root_list)
        root_list->prev = n;
    n->prev = NULL;
    root_list = n;
    long_table_add(roots, handle, n);
}

void unroot(handle_t handle) {
    node_t *n = long_table_remove(roots, handle);
    if (n != NULL) {
        node_t *next = n->next;
        node_t *prev = n->prev;
        if (prev) prev->next = next;
        if (n->prev)
            n->prev->next = n->next;
        free(n);
        if (root_list == n)
            root_list = next;
    }
}
