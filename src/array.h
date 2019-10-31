#pragma once

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <utility>

// TODO: configurable memory allocator (ex: use qxc mem pool for arrays of AST data)
// TODO: template parameter for storing smaller sized arrays on stack
template <typename T>
class DynArray {
    uint8_t* m_data;
    size_t m_length;
    size_t m_capacity;
    static constexpr double s_growth_factor = 1.61803398875;

public:
    DynArray(size_t initial_capacity)
        : m_data(initial_capacity == 0
                     ? nullptr
                     : (uint8_t*)aligned_alloc(alignof(T), sizeof(T) * initial_capacity)),
          m_length(0),
          m_capacity(initial_capacity)
    {
    }

    DynArray(void) : DynArray(0) {}

    // DynArray& operator=(const DynArray&) = delete;
    // DynArray& operator=(DynArray&&) = delete;
    // DynArray(DynArray&&) = delete;
    // DynArray(const DynArray&) = delete;

    ~DynArray(void)
    {
        this->clear();
        free(m_data);
        m_data = nullptr;
        m_length = 0;
        m_capacity = 0;
    }

    T* begin(void) { return reinterpret_cast<T*>(m_data); }
    T* end(void) { return reinterpret_cast<T*>(m_data) + m_length; }
    const T* begin(void) const { return reinterpret_cast<T*>(m_data); }
    const T* end(void) const { return reinterpret_cast<T*>(m_data) + m_length; }

    T& operator[](size_t idx)
    {
        assert(idx < this->length());
        return reinterpret_cast<T*>(m_data)[idx];
    }

    const T& operator[](size_t idx) const
    {
        assert(idx < this->length());
        return reinterpret_cast<T*>(m_data)[idx];
    }

    size_t length(void) const { return m_length; }

    template <typename... Args>
    T* append(Args&&... args)
    {
        if (m_capacity == m_length) {
            const size_t new_capacity =
                m_capacity == 0 ? 2 : (size_t)ceil(s_growth_factor * (double)m_capacity);

            uint8_t* new_data = reinterpret_cast<uint8_t*>(
                aligned_alloc(alignof(T), sizeof(T) * new_capacity));
            T* new_data_cast = reinterpret_cast<T*>(new_data);
            T* old_data_cast = reinterpret_cast<T*>(m_data);

            for (size_t i = 0; i < m_length; i++) {
                new (new_data_cast + i) T(std::move(old_data_cast[i]));
                old_data_cast[i].~T();
            }

            free(m_data);
            m_data = new_data;
            m_capacity = new_capacity;
        }

        T* new_object = reinterpret_cast<T*>(m_data) + m_length;
        new (new_object) T(std::forward<Args>(args)...);
        m_length++;

        return new_object;
    }

    void clear(void)
    {
        T* cast_data = reinterpret_cast<T*>(m_data);
        for (size_t i = 0; i < m_length; i++) {
            cast_data[i].~T();
        }
        m_length = 0;
    }
};

