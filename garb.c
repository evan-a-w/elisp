#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "garb.h"

typedef enum {
    YOUNG,
    OLD,
    KILL,
} age_t;

typedef struct header {
    age_t age;
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

static header_t *header_table = NULL;
static size_t header_table_capacity = 0;
static free_range_t *free_stack = NULL;
static size_t free_stack_size = 0;
static size_t free_stack_capacity = 0;

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
    header_table[h - 1].data = NULL;
    header_table[h - 1].next = NULL_HANDLE;
    free_stack_push(h, h);
}

handle_t get_handle() {
    if (free_stack_size == 0) {
        size_t old_cap = header_table_capacity;
        header_table_capacity = header_table_capacity * 2 + 1;
        header_table = realloc(header_table, HEADER_SIZE * header_table_capacity);
        for (handle_t h = old_cap; h < header_table_capacity; h++)
            header_table[h].data = NULL;
        free_stack_push(old_cap + 1, header_table_capacity);
    } else if (free_stack_size * 2 > header_table_capacity) {
        condense_header_table();
    }
    free_range_t range = free_stack[free_stack_size - 1];
    handle_t res = range.start;
    if (range.start == range.end) {
        free_stack_size--;
    } else {
        free_stack[free_stack_size - 1].start++;
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
    gc_state.young_head = NULL_HANDLE;

    gc_state.old_heap = malloc(OLD_HEAP_DEFAULT);
    if (gc_state.old_heap == NULL) {
        free(gc_state.young_heap);
        gc_state.young_heap = NULL;
        return false;
    }
    gc_state.old_heap_size = 0;
    gc_state.old_head = NULL_HANDLE;
    gc_state.old_heap_capacity = OLD_HEAP_DEFAULT;

    return true;
}

char *allocate_old(size_t size) {
    if (gc_state.old_heap_size + size > gc_state.old_heap_capacity) {
        gc_state.old_heap_capacity = gc_state.old_heap_capacity + (gc_state.old_heap_capacity / 2) + size;
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
    assert(handle);
    header_t *header = get_header(handle);
    assert(header);
    *header = (header_t) {
        .traced = false,
        .age = YOUNG,
        .size = size,
        .data = data,
        .next = next,
        .trace = trace,
        .finalize = finalize,
    };
    return handle;
}

// things should already be traced
bool gc_collect_major(void) {
    handle_t new_head, next;
    new_head = next = NULL_HANDLE;
    size_t survived = 0;
    for (handle_t h = gc_state.old_head; h != NULL_HANDLE; h = next) {
        header_t *header = get_header(h);
        next = header->next;
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
    
    next = NULL_HANDLE;
    for (handle_t h = gc_state.old_head; h != NULL_HANDLE; h = next) {
        header_t *header = get_header(h);
        next = header->next;
        memcpy(new_old, header->data, header->size);
        header->data = new_old;
        new_old += header->size;
    }
    return true;
}

bool gc_collect_minor(void) {
    for_each_root(trace);
    handle_t traced_head = NULL_HANDLE;
    size_t survived = 0;
    handle_t next = NULL_HANDLE;
    for (handle_t curr_handle = gc_state.young_head;
         curr_handle != NULL_HANDLE;
         curr_handle = next
    ) {
        header_t *header = get_header(curr_handle);
        next = header->next;
        if (header->traced) {
            header->age = OLD;
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

    gc_state.bytes_since_major += survived;
    if (gc_state.bytes_since_major > BYTES_TILL_MAJOR_GC) {
        gc_state.bytes_since_major = 0;
        if (!gc_collect_major()) return false;
    }

    handle_t old_head = gc_state.old_head;
    char *chunk = allocate_old(survived);
    next = NULL_HANDLE;
    for (handle_t curr_handle = traced_head;
         curr_handle != NULL_HANDLE;
         curr_handle = next
    ) {
        header_t *header = get_header(curr_handle);
        next = header->next;
        header->traced = false;
        memcpy(chunk, header->data, header->size);
        header->data = chunk;
        chunk += header->size;
        if (header->next == NULL_HANDLE) {
            header->next = gc_state.old_head;
            gc_state.old_head = curr_handle;
        }
    }
    

    for (handle_t curr_handle = old_head;
         curr_handle != NULL_HANDLE;
         curr_handle = get_header(curr_handle)->next
    ) get_header(curr_handle)->traced = false;

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

    header_t *header = get_header(handle);
    if (size > YOUNG_HEAP_SIZE) {
        header->next = gc_state.old_head;
        header->age = OLD;
        gc_state.old_head = handle;
    } else {
        header->age = YOUNG;
        gc_state.young_head = handle;
        gc_state.young_heap_next += size + HEADER_SIZE;
    }

    return handle;
}

void gc_destroy() {
    for (handle_t h = 0; h < header_table_capacity; h++) {
        header_t *header = &header_table[h];
        if (header->data == NULL) continue;
        if (header->finalize) {
            header->finalize(header->data);
        }
    }
    free(header_table);
    free(free_stack);
    free(gc_state.old_heap);
    free(gc_state.young_heap);
}

void trace(handle_t h) {
    if (h) {
        header_t *header = get_header(h);
        if (header->traced) return;
        header->traced = true;
        if (header->trace) header->trace(header->data);
    }
}

void *d(handle_t handle) {
    return get_header(handle)->data;
}
