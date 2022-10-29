#ifndef GARB_H
#define GARB_H

#define YOUNG_HEAP_SIZE 4096
#define OLD_HEAP_DEFAULT 8192
#define BYTES_TILL_MAJOR_GC 8192
#define NULL_HANDLE 0

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define D(h, type) ((type)d(h))

typedef unsigned long handle_t;
typedef struct header header_t;

// Provided by the linked code
void for_each_root(void (*f)(handle_t));

bool gc_init();
void gc_destroy();
void *d(handle_t handle);
void trace(handle_t);
header_t *get_header(handle_t handle);
handle_t galloc(size_t size, void (*trace)(void *), void (*finalize)(void *));
bool gc_collect_minor(void);
bool gc_collect_major(void);

#endif
