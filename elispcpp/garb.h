#ifndef GARB_H
#define GARB_H

#define YOUNG_HEAP_SIZE 16384
#define OLD_HEAP_DEFAULT 16384
#define BYTES_TILL_MAJOR_GC 8192
#define NULL_HANDLE 0

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

enum age_t {
    YOUNG,
    OLD,
    KILL,
};

typedef unsigned long handle_t;
typedef struct header header_t;
typedef void (*hf_t)(handle_t);
typedef void (*handler_t)(void *);

// Provided by the linked code
void for_each_root(void (*f)(handle_t));
void init_roots(void);
void destroy_roots(void);

void set_trace(int tag, void (*trace)(void *));
void set_finalize(int tag, void (*finalize)(void *));

bool gc_init();
void gc_destroy();
template<typename T>
T &d(handle_t handle);
template<typename T>
T *dp(handle_t handle);
void trace(handle_t);
void finalize(handle_t);
void trace_young(handle_t);
void trace_old(handle_t);
header_t *get_header(handle_t handle);
handle_t galloc(size_t size, int tag);
bool gc_collect_minor(void);
bool gc_collect_major(void);
void set_tag(handle_t handle, int tag);
int tag(handle_t h);
void unsafe_set_data(handle_t h, void *val);

#endif
