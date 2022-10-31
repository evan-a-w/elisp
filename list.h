#ifndef LIST_H
#define LIST_H

#include "garb.h"
#include "api.h"

#define L(h) (D((h), list_node_t *))

typedef struct list_node {
    handle_t val;
    handle_t next;
} list_node_t;

handle_t list_new(handle_t val);
handle_t list_cons(handle_t a, handle_t l);
handle_t list_head(handle_t l);
handle_t list_tail(handle_t l);
void list_for_each(handle_t l, void (*f)(handle_t));
void list_finalize(void *l);
void list_trace(void *l);
handle_t list_insert_or(handle_t l, cmp_mod_t *cmp, handle_t (*f)(handle_t, handle_t));
bool list_search(handle_t l, cmp_mod_t *cmp, handle_t *save_to);

#endif
