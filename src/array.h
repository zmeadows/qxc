#pragma once

#include <assert.h>
#include <math.h>

#include <utility>

// TODO: configurable memory allocator (ex: use qxc mem pool for arrays of AST data)
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

template <typename T, typename... Args>
T* array_extend(array<T>* arr, Args&&... args)
{
    if (arr->capacity == arr->length) {
        size_t new_capacity = (size_t)ceil(arr->growth_factor * (double)arr->capacity);
        assert(new_capacity > 0 && new_capacity > arr->capacity);
        arr->data = (T*)realloc(arr->data, sizeof(T) * new_capacity);
        arr->capacity = new_capacity;
    }

    T* new_object = arr->data + arr->length;
    arr->length++;

    new (new_object) T(std::forward<Args>(args)...);

    return new_object;
}

template <typename T>
inline T* array_at(array<T>* arr, size_t idx)
{
    assert(idx < arr->length);
    return arr->data + idx;
}

template <typename T>
inline void array_clear(array<T>* arr)
{
    for (size_t i = 0; i < arr->length; i++) {
        arr->data[i].~T();
    }
    arr->length = 0;
}

template <typename T>
void array_destroy(array<T>* arr)
{
    array_clear(arr);
    free(arr->data);
    arr->data = nullptr;
    arr->length = 0;
    arr->capacity = 0;
}
