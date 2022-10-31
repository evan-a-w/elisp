#ifndef CONST_STRING_H
#define CONST_STRING_H

#include "garb.h"

#define CS(h) (D((h), const_string_t *))

typedef struct {
    size_t len;
    char str[];
} const_string_t;

handle_t cs_new(const char *str);
unsigned long cs_hash(handle_t s);
int cs_cmp(void *, handle_t a, handle_t b);

#endif
