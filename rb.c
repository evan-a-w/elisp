#include <stdlib.h>
#include <stdbool.h>

#include "rb.h"
#include "garb.h"

void rb_trace(void *p) {
    rb_node_t *root = p;
    trace(root->left);
    trace(root->right);
    value_node_t *node = root->values;
    while (node != NULL) {
        trace(node->value);
        node = node->next;
    }
}

void rb_finalise(void *root) {
    value_node_t *node = ((rb_node_t *)root)->values;
    while (node != NULL) {
        value_node_t *next = node->next;
        free(node);
    }
}

handle_t rotate_left(handle_t h) {
    h = rb_copy(h);
    handle_t x = rb_copy(RB(h)->right);
    rb_node_t *a = RB(h);
    rb_node_t *b = RB(x);
    a->right = b->left;
    b->left = h;
    b->colour = a->colour;
    a->colour = RED;
    return x;
}

handle_t rotate_right(handle_t h) {
    h = rb_copy(h);
    handle_t x = rb_copy(RB(h)->left);
    rb_node_t *a = RB(h);
    rb_node_t *b = RB(x);
    a->left = b->right;
    b->right = h;
    b->colour = a->colour;
    a->colour = RED;
    return x;
}

handle_t rb_copy(handle_t node) {
    handle_t new_node = galloc(sizeof(rb_node_t), rb_trace, rb_finalise);
    rb_node_t *n = RB(node);
    *RB(new_node) = (rb_node_t) {
        .key = n->key,
        .values = n->values,
        .colour = n->colour,
        .left = n->left,
        .right = n->right,
    };
    return new_node;
}

handle_t rb_copy_with(handle_t node, colour_t c) {
    handle_t new_node = galloc(sizeof(rb_node_t), rb_trace, rb_finalise);
    rb_node_t *n = RB(node);
    *RB(new_node) = (rb_node_t) {
        .key = n->key,
        .values = n->values,
        .colour = c,
        .left = n->left,
        .right = n->right,
    };
    return new_node;
}

handle_t flip_colours(handle_t m) {
    handle_t n = rb_copy(m);
    rb_node_t *h = RB(m);
    rb_node_t *r = RB(n);
    r->colour = !r->colour;
    handle_t t = rb_copy(r->left);
    r = RB(n);
    RB(n)->left = t;
    t = rb_copy(RB(n)->right);
    r = RB(n);
    r->right = t;
    RB(r->left)->colour = !(RB(r->left)->colour);
    RB(r->right)->colour = !(RB(r->right)->colour);
    RB(r->left)->left = RB(h->left)->left;
    RB(r->left)->right = RB(h->left)->right;
    RB(r->right)->left = RB(h->right)->left;
    RB(r->right)->right = RB(h->right)->right;
    return n;
}

handle_t rb_search(handle_t h, long long key) {
    while (h != NULL_HANDLE && RB(h) != NULL) {
        rb_node_t *x = RB(h);
        int cmp = key - x->key;
        if (cmp < 0) {
            h = x->left;
        } else if (cmp > 0) {
            h = x->right;
        } else {
            return x->values->value;
        }
    }
    return NULL_HANDLE;
}

handle_t new_node(long long key, handle_t value, colour_t colour) {
    handle_t h = galloc(sizeof(rb_node_t), rb_trace, rb_finalise);
    rb_node_t *node = RB(h);
    node->values = malloc(sizeof *node->values);
    node->values->next = NULL;
    node->values->value = value;
    node->key = key;
    node->colour = colour;
    node->left = NULL_HANDLE;
    node->right = NULL_HANDLE;
    return h;
}

bool is_red(handle_t node) {
    if (node == NULL_HANDLE) {
        return false;
    }
    return RB(node)->colour == RED;
}

handle_t rb_insert(handle_t han, long long key, handle_t value) {
    if (han == NULL_HANDLE) return new_node(key, value, RED);
    rb_node_t *h = RB(han);

    if (is_red(h->left) && is_red(h->right)) {
        han = flip_colours(han);
        h = RB(han);
    }

    int cmp = key - h->key;
    if (cmp == 0) {
        value_node_t *v = malloc(sizeof *v);
        v->value = value;
        v->next = h->values;
        handle_t new_node = rb_copy(han);
        RB(new_node)->values = v;
    } else if (cmp < 0) h->left = rb_insert(h->left, key, value);
    else h->right = rb_insert(h->right, key, value);

    if (is_red(h->right) && !is_red(h->left)) {
        han = rotate_left(han);
        h = RB(han);
    }
    if (is_red(h->left) && is_red(RB(h->left)->left)) han = rotate_right(han);

    return han;
}
