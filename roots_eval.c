#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "garb.h"
#include "roots.h"
#include "long_table.h"
#include "vec.h"
#include "eval.h"

typedef struct node {
    long long val;
    struct node *prev;
    struct node *next;
} node_t;

static long_table_t roots = NULL;
static node_t *root_list = NULL;

static ul_vec *root_stack = NULL;
static ul_vec *frames = NULL;

unsigned long roots_size(void) {
    return root_stack->size;
}

unsigned long peek_root(void) {
    return ul_peek(root_stack);
}

void for_each_root(void (*f)(handle_t)) {
    call_on_st_vars(f);
    for (node_t *node = root_list; node != NULL; node = node->next) {
        f(node->val);
    }

    for (ul i = 0; i < root_stack->size; i++) {
        f(root_stack->arr[i]);
    }
}

void pop_roots(int n) {
    root_stack->size -= n;
    if (root_stack->size < 0) root_stack->size = 0;
    ul_downsize(root_stack);
}

unsigned long pop_root() {
    return ul_pop(root_stack);
}

void pop_frame() {
    pop_roots(ul_pop(frames));
}

handle_t push_root_in_frame(handle_t h) {
    push_root(h);
    ++*ul_peek_p(frames);
    return h;
}

void new_frame() {
    ul_push(frames, 0);
}

handle_t push_root(handle_t handle) {
    ul_push(root_stack, handle);
    return handle;
}

bool rooted_global(handle_t h) {
    return long_table_find(roots, h) != NULL;
}

void init_roots(void) {
    roots = long_table_new();
    root_list = NULL;
    root_stack = NULL;
    root_stack = ul_vinit(10);
    frames = ul_vinit(0);
}

void destroy_roots(void) {
    long_table_free(roots);
    ul_vfree(root_stack);
    ul_vfree(frames);
    while (root_list) {
        node_t *next = root_list->next;
        free(root_list);
        root_list = next;
    }
}

handle_t root_global(handle_t handle) {
    if (handle == 0)
        return 0;
    node_t *n = malloc(sizeof *n);
    n->val = handle;
    n->next = root_list;
    if (root_list)
        root_list->prev = n;
    n->prev = NULL;
    root_list = n;
    long_table_add(roots, handle, n);
    return handle;
}

void unroot_global(handle_t handle) {
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

handle_t prof(handle_t h) {
    return push_root_in_frame(h);
}

handle_t pro(handle_t h) {
    return push_root(h);
}
