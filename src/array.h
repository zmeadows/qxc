#pragma once

#include <assert.h>
#include <math.h>

template <typename T>
struct array {
    T* data;
    size_t length;
    size_t capacity;
    double growth_factor;

    T* begin(void) { return data; }
    T* end(void) { return data + length; }
};

template <typename T>
struct array<T> array_create(size_t initial_capacity) {
    struct array<T> arr;

    arr.data = (T*)malloc(sizeof(T) * initial_capacity);
    arr.length = 0;
    arr.capacity = initial_capacity;
    arr.growth_factor = 1.61803398875;

    return arr;
}

template <typename T>
void array_destroy(struct array<T>* arr)
{
    free(arr->data);
    arr->data = nullptr;
    arr->length = 0;
    arr->capacity = 0;
}

template <typename T>
T* array_extend(struct array<T>* arr)
{
    if (arr->capacity == arr->length) {
        size_t new_capacity = (size_t)ceil(arr->growth_factor * (double)arr->capacity);
        assert(new_capacity > 0 && new_capacity > arr->capacity);
        arr->data = (T*)realloc(arr->data, sizeof(T) * new_capacity);
        arr->capacity = new_capacity;
    }

    return arr->data + arr->length++;
}

template <typename T>
T* array_at(struct array<T>* arr, size_t idx)
{
    return arr->data + idx;
}

template <typename T>
void array_clear(struct array<T>* arr)
{
    arr->length = 0;
}
