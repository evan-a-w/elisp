#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "garb.h"

typedef struct header {
    bool traced;
    size_t size;
    void *data;
    handle_t next;
    void (*trace)(void *data);
    void (*finalize)(void *self);
} header_t;

typedef struct free_range {
    // [start, end] is free
    handle_t start;
    handle_t end;
} free_range_t;

#define HEADER_SIZE (sizeof(struct header))

static header_t *header_table;
static size_t header_table_capacity;
static free_range_t *free_stack;
static size_t free_stack_size;
static size_t free_stack_capacity;

static void trace_roots(void);

header_t *get_header(handle_t t) {
    if (t == NULL_HANDLE) {
        return NULL;
    }
    return &header_table[t - 1];
}

void condense_header_table() {
    // TODO
}

void free_stack_push(handle_t start, handle_t end) {
    if (free_stack_size == free_stack_capacity) {
        free_stack_capacity = free_stack_capacity * 2 + 1;
        free_stack = realloc(free_stack, free_stack_capacity * sizeof(free_range_t));
        assert(free_stack);
    }
    free_stack[free_stack_size].start = start - 1;
    free_stack[free_stack_size].end = end - 1;
    free_stack_size++;
}

void free_handle(handle_t h) {
    free_stack_push(h, h);
}

handle_t get_handle() {
    if (free_stack_size == 0) {
        size_t old_cap = header_table_capacity;
        header_table_capacity = header_table_capacity * 2 + 1;
        header_table = realloc(header_table, HEADER_SIZE * header_table_capacity);
        free_stack_push(old_cap, header_table_capacity - 1);
    } else if (free_stack_size * 2 > header_table_capacity) {
        condense_header_table();
    }
    free_range_t range = free_stack[free_stack_size];
    handle_t res = range.start;
    if (range.start == range.end) {
        free_stack_size--;
    } else {
        range.start++;
    }
    return res + 1;
}

static struct gc_state {
    // young_heap should always have size YOUNG_HEAP_SIZE
    char *young_heap; 
    char *young_heap_next;
    handle_t young_head;

    char *old_heap;
    handle_t old_head;
    size_t old_heap_size;
    size_t old_heap_capacity;
    size_t bytes_since_major;
} gc_state = {
    .young_heap = NULL,
    .young_heap_next = NULL,
    .young_head = NULL_HANDLE,
    .old_heap = NULL,
    .old_head = NULL_HANDLE,
    .old_heap_size = 0,
    .old_heap_capacity = 0,
    .bytes_since_major = 0,
};

bool gc_init() {
    if (gc_state.young_heap != NULL) {
        return true;
    }

    gc_state.young_heap = malloc(YOUNG_HEAP_SIZE);
    if (gc_state.young_heap == NULL) {
        return false;
    }
    gc_state.young_heap_next = gc_state.young_heap;

    gc_state.old_heap = malloc(OLD_HEAP_DEFAULT);
    if (gc_state.old_heap == NULL) {
        free(gc_state.young_heap);
        gc_state.young_heap = NULL;
        return false;
    }
    gc_state.old_heap_size = 0;
    gc_state.old_heap_capacity = OLD_HEAP_DEFAULT;
    
    return true;
}

char *allocate_old(size_t size) {
    if (gc_state.old_heap_size + size > gc_state.old_heap_capacity) {
        gc_state.old_heap_capacity = gc_state.old_heap_capacity * 3 / 2 + size;
        gc_state.old_heap = realloc(gc_state.old_heap, gc_state.old_heap_capacity);
        assert(gc_state.old_heap);
    }
    char *res = gc_state.old_heap + gc_state.old_heap_size;
    gc_state.old_heap_size += size;
    return res;
}

static handle_t header_init(
    size_t size,
    handle_t next,
    void (*trace)(void *),
    void (*finalize)(void *),
    void *data
) {
    handle_t handle = get_handle();
    header_t *header = get_header(handle);
    *header = (header_t) {
        .traced = false,
        .size = size,
        .data = data,
        .next = next,
        .trace = trace,
        .finalize = finalize,
    };
    return handle;
}

// things should already be traced
static bool gc_collect_major(void) {
    handle_t new_head = NULL_HANDLE;
    size_t survived = 0;
    for (handle_t h = gc_state.old_head; h != NULL_HANDLE; h = get_header(h)->next) {
        header_t *header = get_header(h);
        if (header->traced) {
            header->traced = false;
            header->next = new_head;
            new_head = h;
            survived += header->size;
        } else {
            if (header->finalize) {
                header->finalize(header->data);
            }
            free_handle(h);
        }
    }

    gc_state.old_head = new_head;
    gc_state.bytes_since_major = 0;
    char *new_old = malloc(survived);
    if (new_old == NULL) {
        return false;
    }
    for (handle_t h = gc_state.old_head; h != NULL_HANDLE; h = get_header(h)->next) {
        header_t *header = get_header(h);
        memcpy(new_old, header->data, header->size);
        header->data = new_old;
        new_old += header->size;
    }
    return true;
}

static bool gc_collect_minor(void) {
    trace_roots();
    handle_t traced_head = NULL_HANDLE;
    handle_t curr_handle = gc_state.young_head;
    size_t survived = 0;
    for (handle_t curr_handle = gc_state.young_head;
         curr_handle != NULL_HANDLE;
         curr_handle = get_header(curr_handle)->next
    ) {
        header_t *header = get_header(curr_handle);
        if (header->traced) {
            header->traced = false;
            header->next = traced_head;
            traced_head = curr_handle;
            survived += header->size;
        } else {
            if (header->finalize) {
                header->finalize(header->data);
            }
            free_handle(curr_handle);
        }
    }

    char *chunk = allocate_old(survived);
    for (handle_t curr_handle = traced_head;
         curr_handle != NULL_HANDLE;
         curr_handle = get_header(curr_handle)->next
    ) {
        header_t *header = get_header(curr_handle);
        memcpy(chunk, header->data, header->size);
        header->data = chunk;
        chunk += header->size;
        if (header->next == NULL_HANDLE) {
            header->next = gc_state.old_head;
            gc_state.old_head = curr_handle;
        }
    }
    
    gc_state.bytes_since_major += survived;
    if (gc_state.bytes_since_major > BYTES_TILL_MAJOR_GC) {
        gc_state.bytes_since_major = 0;
        return gc_collect_major();
    } else {
        for (handle_t curr_handle = gc_state.old_head;
             curr_handle != NULL_HANDLE;
             curr_handle = get_header(curr_handle)->next
        ) {
            header_t *header = get_header(curr_handle);
            header->traced = false;
        }
    }

    gc_state.young_head = NULL_HANDLE;
    gc_state.young_heap_next = gc_state.young_heap;
    return true;
}

handle_t galloc(size_t size, void (*trace)(void *), void (*finalize)(void *)) {
    char *data;
    // If size is too big, we put it on the old heap isntantly
    if (size > YOUNG_HEAP_SIZE) {
        data = allocate_old(size);
    } else {
        // If we don't have enough space in the young heap, we do a minor collection
        if (gc_state.young_heap_next + size > gc_state.young_heap + YOUNG_HEAP_SIZE) {
            gc_collect_minor();
        }
        data = gc_state.young_heap_next;
        gc_state.young_heap_next += size;
    }
    handle_t handle = header_init(size, gc_state.young_head, trace, finalize, data);
    if (handle == NULL_HANDLE) return handle;

    if (size > YOUNG_HEAP_SIZE) {
        get_header(handle)->next = gc_state.old_head;
        gc_state.old_head = handle;
    } else {
        gc_state.young_head = handle;
        gc_state.young_heap_next += size + HEADER_SIZE;
    }

    return handle;
}

void gc_destroy() {
    // leak
}

static void trace_roots(void) {
    for (unsigned long i = 0; i < roots_size; i++) {
        header_t *header = get_header(roots[i]);
        header->traced = true;
        header->trace(header->data);
    }
}
