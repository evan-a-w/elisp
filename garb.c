#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "garb.h"
#include "vec.h"
#include "vec_macro.h"

#define CHECKY

#ifdef CHECKY
typedef struct vals {
    void *data;
    size_t size;
    handle_t h;
    struct vals *next;
} vals;

void push_val(handle_t h);
void checky(void);

static vals *all_vals = NULL;
#endif

typedef struct header {
    int age : 2;
    int traced : 1;
    int tag : 29;
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

typedef free_range_t fr;

#define HEADER_SIZE (sizeof(struct header))

VEC_DEFINE(fr)
VEC_CREATE(fr)

static header_t *header_table = NULL;
static size_t header_table_capacity = 0;
static fr_vec *free_ranges = NULL;

header_t *get_header(handle_t t) {
    if (t == NULL_HANDLE) {
        return NULL;
    }
    return &header_table[t - 1];
}

handle_t get_handle(void);

void condense_header_table() {
    // ul curr_cap = header_table_capacity;
    // header_table_capacity /= 2;
    // for (ul i = header_table_capacity; i < curr_cap; i++) {
    //     header_t *h = &header_table[i];
    //     if (h->data != NULL) {
    //         handle_t nh = get_handle();
    //         *get_header(nh) = *h;
    //     }
    // }
    // header_table = realloc(header_table, header_table_capacity * sizeof(header_t));
}

void trace_only(handle_t h, age_t age) {
    if (h) {
        header_t *header = get_header(h);
        if (header->traced) return;
        if ((age_t)header->age == age) {
            header->traced = true;
            if (header->trace) header->trace(header->data);
        }
    }
}

void free_stack_push(handle_t start, handle_t end) {
    fr_push(free_ranges, (free_range_t){start - 1, end - 1});
}

void free_handle(handle_t h) {
    header_table[h - 1].data = NULL;
    header_table[h - 1].next = NULL_HANDLE;
    free_stack_push(h, h);
}

handle_t get_handle() {
    if (free_ranges->size == 0) {
        size_t old_cap = header_table_capacity;
        header_table_capacity = header_table_capacity * 2 + 1;
        header_table = realloc(header_table, HEADER_SIZE * header_table_capacity);
        assert(header_table);
        for (handle_t h = old_cap; h < header_table_capacity; h++)
            header_table[h].data = NULL;
        free_stack_push(old_cap + 1, header_table_capacity);
    } else if (free_ranges->size * 2 > header_table_capacity) {
        condense_header_table();
    }
    free_range_t *range = fr_peek_p(free_ranges);
    handle_t res = range->start;
    if (range->start == range->end) {
        fr_pop(free_ranges);
    } else {
        range->start += 1;
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

    size_t total_allocated;
    size_t num_allocations;
} gc_state = {
    .young_heap = NULL,
    .young_heap_next = NULL,
    .young_head = NULL_HANDLE,
    .old_heap = NULL,
    .old_head = NULL_HANDLE,
    .old_heap_size = 0,
    .old_heap_capacity = 0,
    .bytes_since_major = 0,
    .total_allocated = 0,
    .num_allocations = 0,
};

bool gc_init() {
    srand(0);
    init_roots();
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

    if (!(free_ranges = fr_vinit(10))) {
        free(gc_state.young_heap);
        free(gc_state.old_heap);
        gc_state.young_heap = NULL;
        gc_state.old_heap = NULL;
        return false;
    }

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
    int tag,
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
        .tag = tag,
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
#ifdef CHECKY
    for_each_root(push_val);
#endif
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
        assert(gc_collect_major());
    } else {
        for (handle_t curr_handle = gc_state.old_head;
             curr_handle != NULL_HANDLE;
             curr_handle = next
        ) {
            header_t *header = get_header(curr_handle);
            next = header->next;
            header->traced = false;
        }
    }

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

    gc_state.young_head = NULL_HANDLE;
    gc_state.young_heap_next = gc_state.young_heap;

#ifdef CHECKY
    checky();
#endif
    return true;
}

handle_t galloct(size_t size, int tag, void (*trace)(void *), void (*finalize)(void *)) {
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
    handle_t handle = header_init(size, gc_state.young_head, tag, trace, finalize, data);
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

    gc_state.num_allocations++;
    gc_state.total_allocated += size;

    return handle;
}

handle_t galloc(size_t size, void (*trace)(void *), void (*finalize)(void *)) {
    return galloct(size, 0, trace, finalize);
}

void gc_destroy() {
    printf(
        "Total allocations: %zu, total bytes allocated: %zu\n",
        gc_state.num_allocations, gc_state.total_allocated
    );
    for (handle_t h = 0; h < header_table_capacity; h++) {
        header_t *header = &header_table[h];
        if (header->data == NULL) continue;
        if (header->finalize) {
            header->finalize(header->data);
        }
    }
    free(header_table);
    free(gc_state.old_heap);
    free(gc_state.young_heap);
    fr_vfree(free_ranges);
    destroy_roots();
}

void trace_old(handle_t h) {
    trace_only(h, OLD);
}

void trace_young(handle_t h) {
    trace_only(h, YOUNG);
}

void finalize(handle_t h) {
    if (h) {
        header_t *header = get_header(h);
        if (header->data && header->finalize) {
            void *d = header->data;
            header->data = NULL;
            header->finalize(d);
        }
    }
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


#ifdef CHECKY
void push_val(handle_t h) {
    header_t *head = get_header(h);
    if (!h) return;
    vals *v = malloc(sizeof(vals));
    v->data = malloc(head->size);
    v->size = head->size;
    memcpy(v->data, D(h, void *), head->size);
    v->h = h;
    v->next = all_vals;
    all_vals = v;
}

void checky() {
    vals *v = all_vals;
    while (v) {
        header_t *head = get_header(v->h);
        if (memcmp(v->data, D(v->h, void *), head->size) != 0) {
            printf("ERROR: at handle %lu\n", v->h);
            exit(1);
        }
        free(v->data);
        vals *next = v->next;
        free(v);
        v = next;
    }
    all_vals = NULL;
}

int tag(handle_t h) {
    if (!h) return 0;
    return get_header(h)->tag;
}

void unsafe_set_data(handle_t h, void *val) {
    get_header(h)->data = val;
}

void set_tag(handle_t handle, int tag) {
    get_header(handle)->tag = tag;
}

#endif
