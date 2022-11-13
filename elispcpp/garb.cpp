#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <assert.h>
#include <vector>

#include "garb.h"

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
} header_t;

struct free_range_t {
    // [start, end] is free
    handle_t start;
    handle_t end;
};

#define HEADER_SIZE (sizeof(struct header))

static std::vector<handler_t> trace_funcs;
static std::vector<handler_t> finalize_funcs;
static std::vector<header> header_table;
static std::vector<free_range_t> free_ranges;

template<typename F, typename T>
void call_if(F *f, T x) {
    if (f) {
        f(x);
    }
}

void set_trace(int tag, handler_t trace) {
    if (tag >= trace_funcs.size()) {
        trace_funcs.resize(tag + 1);
    }
    trace_funcs[tag] = trace;
}

void set_finalize(int tag, handler_t finalize) {
    if (tag >= finalize_funcs.size()) {
        finalize_funcs.resize(tag + 1);
    }
    finalize_funcs[tag] = finalize;
}

handler_t get_trace(int tag) {
    if (tag >= trace_funcs.size()) {
        return NULL;
    }
    return trace_funcs[tag];
}

handler_t get_finalize(int tag) {
    if (tag >= finalize_funcs.size()) {
        return NULL;
    }
    return finalize_funcs[tag];
}

header_t *get_header(handle_t t) {
    if (t == NULL_HANDLE) {
        return NULL;
    }
    return &header_table[t - 1];
}

handle_t get_handle(void);

void condense_header_table() { /* do nothing */ }

void free_stack_push(handle_t start, handle_t end) {
    free_ranges.push_back({start - 1, end - 1});
}

void free_handle(handle_t h) {
    header_table[h - 1].data = NULL;
    header_table[h - 1].next = NULL_HANDLE;
    free_stack_push(h, h);
}

handle_t get_handle() {
    if (free_ranges.size() == 0) {
        long long old_cap = header_table.size();
        header_table.resize(old_cap * 2 + 1);
        free_stack_push(old_cap + 1, header_table.size());
    } else if (free_ranges.size() * 2 > header_table.size()) {
        condense_header_table();
    }
    free_range_t &range = free_ranges.back();
    handle_t res = range.start;
    if (range.start == range.end) {
        free_ranges.pop_back();
    } else {
        range.start += 1;
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

    gc_state.young_heap = (char *)malloc(YOUNG_HEAP_SIZE);
    if (gc_state.young_heap == NULL) {
        return false;
    }
    gc_state.young_heap_next = gc_state.young_heap;
    gc_state.young_head = NULL_HANDLE;

    gc_state.old_heap = (char *)malloc(OLD_HEAP_DEFAULT);
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
        gc_state.old_heap = (char *)realloc(gc_state.old_heap, gc_state.old_heap_capacity);
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
    void *data
) {
    handle_t handle = get_handle();
    assert(handle);
    header_t *header = get_header(handle);
    assert(header);
    *header = (header_t) {
        .age = YOUNG,
        .traced = false,
        .tag = tag,
        .size = size,
        .data = data,
        .next = next,
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
            call_if(get_finalize(header->tag), header->data);
            free_handle(h);
        }
    }

    gc_state.old_head = new_head;
    gc_state.bytes_since_major = 0;
    char *new_old = (char *)malloc(survived);
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
            call_if(get_finalize(header->tag), header->data);
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

handle_t galloc(size_t size, int tag) {
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
    handle_t handle = header_init(size, gc_state.young_head, tag, data);
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

void gc_destroy() {
    printf(
        "Total allocations: %zu, total bytes allocated: %zu\n",
        gc_state.num_allocations, gc_state.total_allocated
    );
    for (auto &header : header_table) {
        if (header.data == NULL) continue;
        call_if(get_finalize(header.tag), header.data);
    }
    header_table.clear();
    free_ranges.clear();
    free(gc_state.old_heap);
    free(gc_state.young_heap);
    destroy_roots();
}

void finalize(handle_t h) {
    if (h) {
        header_t *header = get_header(h);
        if (header->data) {
            void *d = header->data;
            header->data = NULL;
            call_if(get_finalize(header->tag), d);
        }
    }
}

void trace(handle_t h) {
    if (h) {
        header_t *header = get_header(h);
        if (header->traced) return;
        header->traced = true;
        call_if(get_trace(header->tag), header->data);
    }
}

template<typename T>
T &d(handle_t handle) { return *(T *)get_header(handle)->data; }

template<typename T>
T *dp(handle_t handle) { return (T *)get_header(handle)->data; }


#ifdef CHECKY
void push_val(handle_t h) {
    header_t *head = get_header(h);
    if (!h) return;
    vals *v = (vals *)malloc(sizeof(vals));
    v->data = malloc(head->size);
    v->size = head->size;
    memcpy(v->data, dp<void *>(h), head->size);
    v->h = h;
    v->next = all_vals;
    all_vals = v;
}

void checky() {
    vals *v = all_vals;
    while (v) {
        header_t *head = get_header(v->h);
        if (memcmp(v->data, dp<void *>(v->h), head->size) != 0) {
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
