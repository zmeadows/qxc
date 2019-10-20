#pragma once

#include <stdint.h>
#include <stdlib.h>

#define DECLARE_DYNAMIC_ARRAY_NEWTYPE(NEW_TYPE, ITEM_TYPE)           \
    struct NEW_TYPE {                                                \
        ITEM_TYPE* data;                                             \
        size_t capacity;                                             \
        size_t length;                                               \
        double growth_factor;                                        \
    };                                                               \
    int NEW_TYPE##_init(struct NEW_TYPE* arr, size_t init_capacity); \
    ITEM_TYPE* NEW_TYPE##_extend(struct NEW_TYPE* arr);              \
    void NEW_TYPE##_free(struct NEW_TYPE* arr);                      \
    ITEM_TYPE* NEW_TYPE##_at(struct NEW_TYPE* arr, size_t i);        \
    void NEW_TYPE##_clear(struct NEW_TYPE* arr);

#define IMPLEMENT_DYNAMIC_ARRAY_NEWTYPE__(NEW_TYPE, ITEM_TYPE, STORAGE)           \
    STORAGE int NEW_TYPE##_init(struct NEW_TYPE* arr, size_t init_capacity)       \
    {                                                                             \
        arr->growth_factor = 1.61803398875;                                       \
        init_capacity = init_capacity == 0 ? 1 : init_capacity;                   \
        arr->data = (ITEM_TYPE*)malloc(init_capacity * sizeof(ITEM_TYPE));        \
        if (arr->data == NULL) {                                                  \
            arr->capacity = 0;                                                    \
            arr->length = 0;                                                      \
            return -1;                                                            \
        }                                                                         \
        arr->capacity = init_capacity;                                            \
        arr->length = 0;                                                          \
        return 0;                                                                 \
    }                                                                             \
                                                                                  \
    STORAGE ITEM_TYPE* NEW_TYPE##_extend(struct NEW_TYPE* arr)                    \
    {                                                                             \
        if (arr->capacity == arr->length) {                                       \
            size_t new_capacity =                                                 \
                max(max(16, arr->capacity + 1),                                   \
                    (size_t)ceil(arr->growth_factor * (double)arr->capacity));    \
            arr->data =                                                           \
                (ITEM_TYPE*)realloc(arr->data, sizeof(ITEM_TYPE) * new_capacity); \
            arr->capacity = new_capacity;                                         \
        }                                                                         \
                                                                                  \
        return &arr->data[arr->length++];                                         \
    }                                                                             \
                                                                                  \
    STORAGE void NEW_TYPE##_free(struct NEW_TYPE* arr)                            \
    {                                                                             \
        free(arr->data);                                                          \
        arr->data = NULL;                                                         \
        arr->length = 0;                                                          \
        arr->capacity = 0;                                                        \
    }                                                                             \
                                                                                  \
    STORAGE void NEW_TYPE##_clear(struct NEW_TYPE* arr) { arr->length = 0; }      \
                                                                                  \
    STORAGE ITEM_TYPE* NEW_TYPE##_at(struct NEW_TYPE* arr, size_t i)              \
    {                                                                             \
        return &arr->data[i];                                                     \
    }

#define IMPLEMENT_DYNAMIC_ARRAY_NEWTYPE(NEW_TYPE, ITEM_TYPE) \
    IMPLEMENT_DYNAMIC_ARRAY_NEWTYPE__(NEW_TYPE, ITEM_TYPE, )

