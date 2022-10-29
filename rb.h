#ifndef RB_H
#define RB_H

// based on https://sedgewick.io/wp-content/themes/sedgewick/papers/2008LLRB.pdf
// and made immutable

#include "garb.h"

typedef enum colour { RED, BLACK } colour_t;

typedef struct value_node {
    void *value;
    struct value_node *next;
} value_node_t;

typedef struct rb_node {
    colour_t colour;
    long long key;
    value_node_t *values;
    struct rb_node *left;
    struct rb_node *right;
} rb_node_t;

void *rb_search(rb_node_t *root, long long key);
rb_node_t *rb_insert(rb_node_t *root, long long key, void *value);

void *free_value_node(value_node_t *node);
void *free_value(void *value);
rb_node_t *new_node(long long key, void *value, colour_t colour);
rb_node_t *copy_node(rb_node_t *node, colour_t c);

#endif
