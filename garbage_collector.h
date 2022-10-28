#ifndef GARBABGE_COLLECTOR_H
#define GARBABGE_COLLECTOR_H

#define YOUNG_HEAP_SIZE 4096
#define BYTES_TILL_MAJOR_GC 8192

#include <stdint.h>
#include <stdio.h>

// Provided by the linked code
extern char ***roots;
extern size_t roots_size;

bool gc_init();

// Returns a double pointer - dereference to get the address of the data
// (allows generational heap to work easily)
// Ensure to trace all local variables before calling
void **galloc(size_t size, void (*trace)(void *), void (*finalize)(void *));

void gc_destroy();

#endif
