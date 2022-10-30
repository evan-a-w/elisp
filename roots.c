#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "garb.h"
#include "roots.h"
#include "long_table.h"

typedef struct node {
    long long val;
    struct node *prev;
    struct node *next;
} node_t;

static long_table_t roots = NULL;
static node_t *root_list = NULL;

static long long *frames = NULL;
static long long frames_size = 0;
static long long frames_capacity = 0;

static handle_t *root_stack = NULL;
static long long root_stack_size = 0;
static long long root_stack_capacity = 0;

void for_each_root(void (*f)(handle_t)) {
    for (node_t *node = root_list; node != NULL; node = node->next) {
        f(node->val);
    }

    for (long long i = 0; i < root_stack_size; i++) {
        f(root_stack[i]);
    }
}

void pop_roots(int n) {
    root_stack_size -= n;
    if (root_stack_size < 0) root_stack_size = 0;
}

void pop_root() {
    if (root_stack_size == 0) return;
    root_stack_size--;
}

void pop_frame() {
    if (frames_size == 0) return;
    frames_size--;
    pop_roots(frames[frames_size]);
}

handle_t push_root_in_frame(handle_t h) {
    push_root(h);
    frames[frames_size - 1]++;
    return h;
}

void new_frame() {
    if (frames_size == frames_capacity) {
        frames_capacity = frames_capacity * 2 + 1;
        frames = realloc(frames, frames_capacity * sizeof(long long));
    }

    frames[frames_size++] = 0;
}

handle_t push_root(handle_t handle) {
    if (root_stack_size == root_stack_capacity) {
        root_stack_capacity = root_stack_capacity * 2 + 1;
        root_stack = realloc(root_stack, root_stack_capacity * sizeof(handle_t));
    }
    root_stack[root_stack_size++] = handle;
    return handle;
}

bool rooted_global(handle_t h) {
    return long_table_find(roots, h) != NULL;
}

void init_roots(void) {
    roots = long_table_new();
    root_list = NULL;
    root_stack = NULL;
    root_stack_size = 0;
    root_stack_capacity = 0;
    frames = NULL;
    frames_size = 0;
    frames_capacity = 0;
}

void destroy_roots(void) {
    long_table_free(roots);
    free(root_stack);
    free(frames);
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
