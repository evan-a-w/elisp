#include <string.h>

#include "garb.h"
#include "const_string.h"
#include "api.h"

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

int cs_cmp(void *ignore, handle_t a, handle_t b) {
    (void)ignore;
    if ((tag(a) != STR && tag(a) != SYMBOL) || (tag(b) != STR && tag(b) != SYMBOL)) {
        printf("Warning: comparing handles of different types (expected STR, got %d and %d)\n", tag(a), tag(b));
    }
    return strcmp(CS(a)->str, CS(b)->str);
}
