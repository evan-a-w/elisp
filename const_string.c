#include <string.h>

#include "garb.h"
#include "const_string.h"
#include "base_types.h"

typedef unsigned long ul;

handle_t cs_new(const char *str) {
    size_t len = strlen(str);
    handle_t h = galloct(sizeof(const_string_t) + len + 1, STR, NULL, NULL);
    const_string_t *cs = CS(h);
    cs->len = len;
    memcpy(cs->str, str, len + 1);
    return h;
}

// from https://stackoverflow.com/questions/7666509/hash-function-for-string
ul cs_hash(handle_t h) {
    const char *str = CS(h)->str;
    ul hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int cs_cmp(void *_, handle_t a, handle_t b) {
    if (!TAG_EQ(a, b, STR)) return -1;
    return strcmp(CS(a)->str, CS(b)->str);
}
