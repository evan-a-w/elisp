#ifndef GARB_H
#define GARB_H

#define YOUNG_HEAP_SIZE 4096
#define OLD_HEAP_DEFAULT 8192
#define BYTES_TILL_MAJOR_GC 8192
#define NULL_HANDLE 0

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef unsigned long handle_t;
typedef struct header header_t;

// Provided by the linked code
extern handle_t *roots;
extern size_t roots_size;

bool gc_init();

void *d(handle_t handle);
header_t *get_header(handle_t handle);

handle_t galloc(size_t size, void (*trace)(void *), void (*finalize)(void *));

void gc_destroy();

#endif
