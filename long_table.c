#include "long_table.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>

#define NUM_BENCH 1000000
#define NUM_BENCH_NUMS 10
#define NUM_BENCH_TRIES 5

unsigned long hashl(unsigned long x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;

    x = x ^ (x >> 31);
    return x;
}

void *kvl_pair_delete(kvl_pair_t p) {
    void *val = p->val;
    free(p);
    return val;
}

bool is_prime(unsigned long x) {
    unsigned long lim = floor(sqrt(x));
    for (unsigned long i = 2; i <= lim; i++) {
        if (x % i == 0)
            return false;
    }
    return true;
}

unsigned long nearest_prime(unsigned long x) {
    if (x < 2) return 2;
    unsigned long res;
    for (res = x; ; res++) {
        if (is_prime(res))
            break;
    }
    return res;
}

void long_table_realloc(long_table_t table, unsigned long new_capacity) {
    kvl_pair_t *new_boxes = calloc(new_capacity, sizeof(kvl_pair_t));
    for (unsigned long i = 0; i < table->capacity; i++) {
        if (table->boxes[i] != NULL) {
            unsigned long key = table->boxes[i]->key;
            unsigned long hashed = hashl(key);
            unsigned long index = hashed % new_capacity;
            int j;
            for (j = index; new_boxes[j]; j = (j + 1) % new_capacity)
                ;
            new_boxes[j] = table->boxes[i];
        }
    }
    free(table->boxes);
    table->boxes = new_boxes;
    table->capacity = new_capacity;
}

// Takes ownership of val (needs to be a valid pointer), but copies key
// Returns old val if present, else NULL.
void *long_table_add(long_table_t table, unsigned long key, void *val) {
    table->size++;
    unsigned long hashed = hashl(key);
    unsigned long index = hashed % table->capacity;
    void *res = NULL;
    int i;
    for (i = index; table->boxes[i]; i = (i + 1) % table->capacity) {
        if (table->boxes[i]->key == key) {
            res = kvl_pair_delete(table->boxes[i]);
            table->size--;
            break;
        }
    }
    table->boxes[i] = malloc(sizeof(*table->boxes[i]));
    table->boxes[i]->key = key;
    table->boxes[i]->val = val;

    double load = ((double) table->size) / (double) table->capacity;
    if (load > MAX_LOAD)
        long_table_realloc(table, nearest_prime(table->capacity * TABLE_REALLOC_FACTOR));

    return res;
}

long_table_t long_table_new() {
    long_table_t table = malloc(sizeof *table);
    table->size = 0;
    table->capacity = nearest_prime(DEFAULT_CAPACITY);
    table->boxes = calloc(table->capacity, sizeof(kvl_pair_t));
    return table;
}

void long_table_free(long_table_t table) {
    if (table) {
        for (unsigned long i = 0; i < table->capacity; i++) {
            if (table->boxes[i] != NULL) {
                free(table->boxes[i]);
            }
        }
        free(table->boxes);
        free(table);
    }
}

kvl_pair_t long_table_find(long_table_t table, unsigned long key) {
    unsigned long hashed = hashl(key);
    unsigned long index = hashed % table->capacity;


    for (int i = index; table->boxes[i]; i = (i + 1) % table->capacity) {
        if (table->boxes[i]->key == key) {
            return table->boxes[i];
        }
    }

    return NULL;
}

void *long_table_remove(long_table_t table, unsigned long key) {
    int i = hashl(key) % table->capacity;
    for (;;) {
        if (table->boxes[i] == NULL)
            return NULL;
        if (table->boxes[i]->key == key)
            break;
        i = (i + 1) % table->capacity;
    }
    void *res = kvl_pair_delete(table->boxes[i]);
    table->boxes[i] = NULL;
    table->size--;

    i = (i + 1) % table->capacity;
    while (table->boxes[i]) {
        void *val = table->boxes[i]->val;
        unsigned long tkey = table->boxes[i]->key;
        free(table->boxes[i]);
        table->boxes[i] = NULL;
        table->size--;
        long_table_add(table, tkey, val);
        i = (i + 1) % table->capacity;
    }

    double load = ((double) table->size) / (double) table->capacity;
    if (load < MIN_LOAD)
        long_table_realloc(table, nearest_prime(table->size + table->size / 2));

    return res;
}
