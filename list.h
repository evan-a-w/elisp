#ifndef LIST_H
#define LIST_H

#include "garb.h"

#define L(h) (D((h), list_node_t *))

typedef struct list_node {
    handle_t val;
    handle_t next;
} list_node_t;

handle_t list_new(handle_t val);
handle_t list_cons(handle_t a, handle_t l);
handle_t list_head(handle_t l);
handle_t list_tail(handle_t l);

#endif
