#ifndef VEC_MACRO_H
#define VEC_MACRO_H

#define VEC_DEFINE(TYPE) \
    typedef struct TYPE##_Vec {\
        TYPE * arr;\
        unsigned long size;\
        unsigned long capacity;\
    } TYPE##_vec;\
    TYPE##_vec * TYPE##_vinit(int start_cap); \
    void TYPE##_downsize(TYPE##_vec *vec);\
    void TYPE##_push(TYPE##_vec *vec, TYPE val); \
    TYPE TYPE##_pop(TYPE##_vec *vec); \
    void TYPE##_vfree(TYPE##_vec *vec); \
    TYPE * TYPE##_peek_p(TYPE##_vec *vec);\
    TYPE TYPE##_peek(TYPE##_vec *vec);

#define VEC_CREATE(TYPE) \
    TYPE##_vec * TYPE##_vinit(int start_cap) {\
        TYPE##_vec *vec = (TYPE##_vec *) malloc(sizeof(TYPE##_vec)); \
        vec->arr = (TYPE *) malloc(start_cap * sizeof(TYPE));\
        vec->capacity = start_cap;\
        vec->size = 0;\
        return vec;\
    }\
    void TYPE##_downsize(TYPE##_vec *vec) {\
        if (2 * vec->size + 1 < vec->capacity) {\
            vec->capacity = vec->capacity / 2 + 1;\
            vec->arr = (TYPE *) realloc(vec->arr, vec->capacity * sizeof(TYPE));\
        }\
    }\
    void TYPE##_push(TYPE##_vec *vec, TYPE val) {\
        if (vec->size >= vec->capacity) {\
            vec->capacity = vec->capacity * 2 + 1;\
            vec->arr = (TYPE *) realloc(vec->arr, vec->capacity * sizeof(TYPE));\
        }\
        (vec->arr)[vec->size++] = val;\
    }\
    TYPE TYPE##_pop(TYPE##_vec *vec) {\
        assert(vec->size > 0);\
        TYPE##_downsize(vec);\
        return vec->arr[--vec->size];\
    }\
    void TYPE##_vfree(TYPE##_vec *vec) {\
        free(vec->arr);\
        free(vec);\
    }\
    TYPE TYPE##_peek(TYPE##_vec *vec) {\
        assert(vec->size > 0);\
        return vec->arr[vec->size - 1];\
    }\
    TYPE * TYPE##_peek_p(TYPE##_vec *vec) {\
        assert(vec->size > 0);\
        return &vec->arr[vec->size - 1];\
    }
#endif
