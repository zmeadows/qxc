#pragma once

#include <math.h>

template <typename T, size_t N>
struct sdarray {
    T stack_data[N];
    T* data;
    size_t length;
    size_t capacity;
    double growth_factor;

    static struct sdarray<T, N> create(void) {
        struct sdarray<T, N> arr;

        arr.data = arr.stack_data;
        arr.length = 0;
        arr.capacity = N;
        arr.growth_factor = 1.61803398875;

        return arr;
    };

    static void destroy(struct sdarray<T, N>* arr)
    {
        if (arr->data != arr->stack_data) {
            free(arr->data);
            arr->data = arr->stack_data;
            arr->length = 0;
            arr->capacity = N;
        }
    };

    T* begin(void) { return data; }
    T* end(void) { return data + length; }
};

template <typename T, size_t N>
T* sdarray_extend(struct sdarray<T, N>* arr)
{
    if (arr->capacity == arr->length) {
        size_t new_capacity = (size_t)ceil(arr->growth_factor * (double)arr->capacity);
        assert(new_capacity > 0 && new_capacity > arr->capacity);

        if (arr->data == arr->stack_data) {
            arr->data = (T*)malloc(sizeof(T) * new_capacity);
            for (size_t i = 0; i < arr->length; i++) {
                arr->data[i] = arr->stack_data[i];
            }
        }
        else {
            // already heap allocated
            arr->data = (T*)realloc(arr->data, sizeof(T) * new_capacity);
        }

        arr->capacity = new_capacity;
    }

    return arr->data + arr->length++;
}
