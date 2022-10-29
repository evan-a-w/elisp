#include <stdlib.h>
#include <stdbool.h>

#include "rb.h"
#include "garb.h"

rb_node_t *rotate_left(rb_node_t *h) {
    h = copy_node(h, h->colour);
    rb_node_t *x = copy_node(h->right, h->right->colour);
    h->right = x->left;
    x->left = h;
    x->colour = h->colour;
    h->colour = RED;
    return x;
}

rb_node_t *rotate_right(rb_node_t *h) {
    h = copy_node(h, h->colour);
    rb_node_t *x = copy_node(h->left, h->left->colour);
    h->left = x->right;
    x->right = h;
    x->colour = h->colour;
    h->colour = RED;
    return x;
}

rb_node_t *copy_node(rb_node_t *node, colour_t c) {
    rb_node_t *new_node = malloc(sizeof(rb_node_t));
    *new_node = (rb_node_t) {
        .key = node->key,
        .values = node->values,
        .colour = c,
        .left = node->left,
        .right = node->right,
    };
    return new_node;
}

rb_node_t *flip_colours(rb_node_t *h) {
    rb_node_t *r = copy_node(h, !h->colour);
    r->left = copy_node(h->left, !h->left->colour);
    r->right = copy_node(h->right, !h->right->colour);
    r->left->left = h->left->left;
    r->left->right = h->left->right;
    r->right->left = h->right->left;
    r->right->right = h->right->right;
    return r;
}

void *rb_search(rb_node_t *root, long long key) {
    rb_node_t *x = root;
    while (x != NULL) {
        int cmp = key - x->key;
        if (cmp < 0) {
            x = x->left;
        } else if (cmp > 0) {
            x = x->right;
        } else {
            return x->values->value;
        }
    }
    return NULL;
}

rb_node_t *new_node(long long key, void *value, colour_t colour) {
    rb_node_t *node = malloc(sizeof(rb_node_t));
    node->values = malloc(sizeof *node->values);
    node->values->next = NULL;
    node->values->value = value;
    node->key = key;
    node->colour = colour;
    node->left = NULL;
    node->right = NULL;
    return node;
}

bool is_red(rb_node_t *node) {
    if (node == NULL) {
        return false;
    }
    return node->colour == RED;
}

rb_node_t *rb_insert(rb_node_t *h, long long key, void *value) {
    if (h == NULL) return new_node(key, value, RED);

    if (is_red(h->left) && is_red(h->right)) h = flip_colours(h);

    int cmp = key - h->key;
    if (cmp == 0) {
        value_node_t *v = malloc(sizeof *v);
        v->value = value;
        v->next = h->values;
        rb_node_t *new_node = copy_node(h, h->colour);
        new_node->values = v;
    } else if (cmp < 0) h->left = rb_insert(h->left, key, value);
    else h->right = rb_insert(h->right, key, value);

    if (is_red(h->right) && !is_red(h->left)) h = rotate_left(h);
    if (is_red(h->left) && is_red(h->left->left)) h = rotate_right(h);

    return h;
}
