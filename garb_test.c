#include <stdio.h>
#include <assert.h>

#include "garb.h"
#include "root_stack.h"

typedef struct node {
    long long val;
    handle_t next;
} node_t;

void trace_node(void *data) {
    node_t *node = data;
    trace(node->next);
}

void insert_front(handle_t *list, long long val) {
    handle_t h = galloc(sizeof(node_t), trace_node, NULL);
    if (*list != 0) pop_root();
    push_root(h);
    D(h, node_t *)->val = val;
    D(h, node_t *)->next = *list;
    *list = h;
}

handle_t new_node(long long val, handle_t next) {
    handle_t h = galloc(sizeof(node_t), trace_node, NULL);
    push_root(h);
    node_t *node = d(h);
    node->val = val;
    node->next = next;
    return h;
}

int main(void) {
    gc_init();

    for (int i = 1; i <= 40; i++) {
        handle_t h = galloc(sizeof(int), NULL, NULL);
        push_root(h);
        *D(h, int *) = i;
    }
    
    handle_t head = NULL_HANDLE;
    for (int i = 0; i < 100; i++) {
        insert_front(&head, i);
        for (handle_t h = 1; h <= 40; h++) {
            if (*D(h, int *) != (int)h)
                printf("Error: %lu = %d\n", h, *D(h, int *));
            assert(*D(h, int *) == (int)h);
        }
    }

    for (handle_t h = 1; h <= 40; h++) {
        assert(*D(h, int *) == (int)h);
    }


    long long v = 99;
    while (head != NULL_HANDLE) {
        assert(D(head, node_t *)->val == v);
        v--;
        head = D(head, node_t *)->next;
    }

    pop_root();

    gc_collect_minor();


    for (int i = 0; i < 100000; i++) {
        assert(galloc(10, NULL, NULL) != NULL_HANDLE);
    }

    gc_destroy();
}
