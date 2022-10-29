#ifndef RB_H
#define RB_H

// based on https://sedgewick.io/wp-content/themes/sedgewick/papers/2008LLRB.pdf
// and made immutable

#include "garb.h"

#define RB(h) (D(h, rb_node_t *))

typedef enum colour { RED, BLACK } colour_t;

typedef struct value_node {
    handle_t value;
    struct value_node *next;
} value_node_t;

typedef struct rb_node {
    colour_t colour;
    long long key;
    value_node_t *values;
    handle_t left;
    handle_t right;
} rb_node_t;

handle_t rb_search(handle_t root, long long key);
handle_t rb_insert(handle_t root, long long key, handle_t value);

handle_t new_node(long long key, handle_t value, colour_t colour);
handle_t rb_copy(handle_t root);
handle_t rb_copy_with(handle_t root, colour_t c);

void rb_trace(void *);
void rb_finalise(void *);

#endif
