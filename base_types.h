#ifndef BASE_TYPES_H
#define BASE_TYPES_H

#include "garb.h"
#include "parser.h"

#define I(h) (D((h), long long))

typedef enum {
    NON_USER,
    SPEC,
    SEXPR,
    SYMBOL,
    INT,
    BOOL,
    STR,
    LIST,
    MAP,
    HPAIR,
    ENV,
} type_tag_t;

#define TAG_EQ(a, b, t) ((a) == NULL_HANDLE || (b) == NULL_HANDLE || ((tag((a)) == tag((b))) && tag((a)) == (t)))

#endif
