#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "garbage_collector.h"

#define HEADER_SIZE (sizeof(struct header))

typedef struct header {
    size_t size;
    // Only used if it's in the large young heap
    struct header *next;
    void (*trace)(void *data);
    void (*finalize)(void *self);
    void **data;
    bool traced;
} header_t;

typedef struct gc_state {
    // young_heap should always have size YOUNG_HEAP_SIZE
    char *young_heap; 
    char *young_heap_next;
    struct header *large_young_heap;

    char *old_heap;
    size_t old_heap_size;
    size_t bytes_since_major;
} gc_state_t;

static void trace_roots(void);

gc_state_t gc_state;

bool gc_init() {
    gc_state.young_heap = malloc(YOUNG_HEAP_SIZE);
    if (gc_state.young_heap == NULL) {
        return false;
    }
    gc_state.young_heap_next = gc_state.young_heap;
    gc_state.large_young_heap = NULL;

    gc_state.old_heap = NULL;
    gc_state.old_heap_size = 0;
    
    return true;
}

static bool header_init(header_t *header,
                        size_t size,
                        header_t *next,
                        void (*trace)(void *),
                        void (*finalize)(void *),
                        void *data)
{
    header->size = size;
    header->next = next;
    header->trace = trace;
    header->finalize = finalize;
    header->traced = false;

    header->data = malloc(sizeof(*header->data));
    if (header->data == NULL)
        return false;
    header->data[0] = data;

    return true;
}

static bool gc_collect_major(void) {
    // clear traced statuses
    header_t *header = (header_t *)gc_state.old_heap;
    while ((char *)header < gc_state.old_heap + gc_state.old_heap_size) {
        header->traced = false;

        header = (header_t *)((char *)header + header->size);
    }
    trace_roots();

    header = (header_t *)gc_state.old_heap;
    header_t *surviving = NULL;
    size_t surviving_size = 0;
    while ((char *)header < gc_state.old_heap + gc_state.old_heap_size) {
        if (header->traced) {
            header->next = surviving;
            surviving = header;
            header->traced = false;
            surviving_size += header->size + HEADER_SIZE;
        } else {
            free(header->data);
        }

        header = (header_t *)((char *)header + header->size);
    }

    char *new_heap = malloc(gc_state.old_heap_size);
    char *next_new_heap = new_heap;
    if (new_heap == NULL)
        return false;

    for (header_t *curr = surviving; curr; curr = curr->next) {
        memcpy(next_new_heap, curr, curr->size + HEADER_SIZE);
        header_t *new_header = (header_t *)next_new_heap;
        *(new_header->data) = (char *)new_header + HEADER_SIZE;
        next_new_heap += curr->size + HEADER_SIZE;
    }

    free(gc_state.old_heap);

    gc_state.old_heap_size = surviving_size;
    gc_state.old_heap = new_heap;

    gc_state.bytes_since_major = 0;

    return true;
}

static bool gc_collect_minor(void) {
    trace_roots();
    header_t *curr = (header_t *)gc_state.young_heap;
    header_t *surviving = NULL;
    size_t surviving_bytes = 0;

    while ((char *)curr < gc_state.young_heap_next) {
        header_t *next = (header_t *)((char *)curr + HEADER_SIZE + curr->size);
        if (curr->traced) {
            curr->next = surviving;
            curr->traced = false;
            surviving = curr;
            surviving_bytes += curr->size + HEADER_SIZE;
        } else {
            free(curr->data);
            curr->finalize((char *)curr + HEADER_SIZE);
        }

        curr = next;
    }
    
    curr = gc_state.large_young_heap;
    while (curr) {
        header_t *next = curr->next;

        // Leave traced to indicate large val for later in this function
        if (curr->traced) {
            curr->next = surviving;
            surviving = curr;
            surviving_bytes += curr->size + HEADER_SIZE;
        } else {
            free(curr->data);
            curr->finalize((char *)curr + HEADER_SIZE);
            free(curr);
        }

        curr = next;
    }

    gc_state.young_heap_next = gc_state.young_heap;
    gc_state.large_young_heap = NULL;

    gc_state.bytes_since_major += surviving_bytes;
    if (gc_state.bytes_since_major > BYTES_TILL_MAJOR_GC) {
        bool res = gc_collect_major();
        if (!res)
            return res;
    }

    size_t new_old_size = gc_state.old_heap_size + surviving_bytes;
    char *new_old_heap = realloc(gc_state.old_heap, new_old_size);
    if (new_old_heap == NULL) {
        // big failure not enough memory. can deal with a bit anyway but not yet
        return false;
    }

    gc_state.old_heap_size = new_old_size;
    gc_state.old_heap = new_old_heap;
    char *old_heap_next = gc_state.old_heap;
    header_t *next = NULL;
    for (curr = surviving; curr; curr = next) {
        next = curr->next;

        bool traced = curr->traced;
        curr->traced = false;

        memcpy(old_heap_next, curr, HEADER_SIZE + curr->size);
        header_t *new_header = (header_t *)old_heap_next;
        *(new_header->data) = (char *)new_header + HEADER_SIZE;
        old_heap_next += HEADER_SIZE + curr->size;

        if (traced)
            free(curr);
    }

    return true;
}

void **galloc(size_t size, void (*trace)(void *), void (*finalize)(void *)) {
    // If size is too big, we just use malloc
    if (size > YOUNG_HEAP_SIZE) {
        header_t *header = malloc(size + HEADER_SIZE);
        if (header == NULL)
            return NULL;
        if (header_init(header, size, gc_state.large_young_heap, trace, finalize,
                        (char *)header + HEADER_SIZE)) {
            free(header);
            return NULL;
        }
        gc_state.large_young_heap = header;

        return header->data;
    }

    // If we're out of space, we need to collect
    if (gc_state.young_heap_next + size > gc_state.young_heap + HEADER_SIZE) {
        gc_collect_minor();
    }

    // Now there should be enough space
    header_t *header = (header_t *)gc_state.young_heap_next;
    if (header_init(header, size, NULL, trace, finalize, (char *)header + HEADER_SIZE))
        return NULL;
    gc_state.young_heap_next += size + HEADER_SIZE;

    return header->data;
}

void gc_destroy() {
    // leak
}

static void trace_roots(void) {
    for (int i = 0; i < roots_size; i++) {
        header_t *header = (header_t *)(*roots[i] - HEADER_SIZE);
        header->traced = true;
        header->trace((char *)header + HEADER_SIZE);
    }
}
