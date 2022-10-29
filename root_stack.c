#include <stdlib.h>
#include <assert.h>

#include "garb.h"
#include "root_stack.h"

static handle_t *roots = NULL;
static size_t roots_size = 0;
static size_t roots_capacity = 0;

void push_root(handle_t root) {
    if (roots_size == roots_capacity) {
        roots_capacity = roots_capacity * 2 + 1;
        roots = realloc(roots, roots_capacity * sizeof(handle_t));
        assert(roots);
    }
    roots[roots_size++] = root;
}

void pop_root() {
    if (roots_size == 0) {
        return;
    }
    roots_size--;
}

void free_root_stack() {
    free(roots);
}

void for_each_root(void (*f)(handle_t)) {
    for (size_t i = 0; i < roots_size; i++) f(roots[i]);
}
