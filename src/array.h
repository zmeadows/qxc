#pragma once

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <utility>

template <typename T, size_t N>
struct DynArray {
    alignas(alignof(T)) uint8_t stack_data[sizeof(T) * N];
    uint8_t* data;
    size_t length;
    size_t capacity;
    static constexpr double growth_factor = 1.61803398875;

    T* begin(void) { return (T*)data; }
    T* end(void) { return ((T*)data) + length; }

    T& operator[](size_t idx)
    {
        assert(idx < length);
        return ((T*)data)[idx];
    }
};

template <typename T>
struct DynArray<T, 0> {
    uint8_t* data;
    size_t length;
    size_t capacity;
    static constexpr double growth_factor = 1.61803398875;

    T* begin(void) { return (T*)data; }
    T* end(void) { return ((T*)data) + length; }

    T& operator[](size_t idx)
    {
        assert(idx < length);
        return ((T*)data)[idx];
    }
};

template <typename T>
using DynHeapArray = DynArray<T, 0>;

template <typename T, size_t N>
DynArray<T, N> array_create(void)
{
    DynArray<T, N> arr;
    arr.data = arr.stack_data;
    arr.length = 0;
    arr.capacity = N;
    return arr;
}

template <typename T>
DynArray<T, 0> heap_array_create(size_t initial_capacity = 0)
{
    DynHeapArray<T> arr;

    if (initial_capacity > 0) {
        arr.data = (uint8_t*)(aligned_alloc(alignof(T), sizeof(T) * initial_capacity));
    }
    else {
        arr.data = nullptr;
    }

    arr.length = 0;
    arr.capacity = initial_capacity;

    return arr;
}

template <typename T, size_t N>
bool on_stack(DynArray<T, N>* arr)
{
    return arr->data == arr->stack_data;
}

template <typename T>
constexpr bool on_stack(DynArray<T, 0>*)
{
    return false;
}

template <typename T, size_t N>
void reserve(DynArray<T, N>* arr, size_t new_capacity)
{
    if (new_capacity > arr->capacity) {
        uint8_t* new_data =
            (uint8_t*)(aligned_alloc(alignof(T), sizeof(T) * new_capacity));

        memcpy(new_data, arr->data, sizeof(T) * arr->length);
        if (!on_stack(arr)) {
            free(arr->data);
        }
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
}

template <typename T, size_t N>
void array_free(DynArray<T, N>* arr)
{
    if (arr->data != arr->stack_data) {
        free(arr->data);
    }
    arr->data = arr->stack_data;
    arr->length = 0;
    arr->capacity = N;
}

template <typename T>
void array_free(DynArray<T, 0>* arr)
{
    free(arr->data);
    arr->data = nullptr;
    arr->length = 0;
    arr->capacity = 0;
}

// returns pointer to new uninitialized element at the end of the array
// if already at the max capacity, extends capacity of array
// potentially switching from stack to heap
template <typename T, size_t N>
T* array_extend(DynArray<T, N>* arr)
{
    if (arr->capacity == arr->length) {
        size_t new_capacity =
            std::max((size_t)2, (size_t)ceil(arr->growth_factor * (double)arr->capacity));
        reserve(arr, new_capacity);
    }

    T* new_object = ((T*)arr->data) + arr->length;
    arr->length++;

    return new_object;
}

// returns pointer to new uninitialized element at the end of the array
// if already at the max capacity, extends capacity of array
// potentially switching from stack to heap
template <typename T, size_t N, typename... Args>
void array_append(DynArray<T, N>* arr, Args&&... args)
{
    if (arr->capacity == arr->length) {
        size_t new_capacity =
            std::max((size_t)2, (size_t)ceil(arr->growth_factor * (double)arr->capacity));
        reserve(arr, new_capacity);
    }

    T* new_object = ((T*)arr->data) + arr->length;
    new (new_object) T(std::forward<Args>(args)...);
    arr->length++;
}

template <typename T, size_t N>
void array_clear(DynArray<T, N>* arr)
{
    arr->length = 0;
}
