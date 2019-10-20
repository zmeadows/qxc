#pragma once

#include <stddef.h>
#include <stdint.h>

struct qxc_memory_pool {
    struct qxc_memory_arena_chain* chain_tip;
    size_t arena_size;
};

struct qxc_memory_arena_chain {
    uint8_t* start;
    uint8_t* end;
    uint8_t* bump_ptr;
    struct qxc_memory_arena_chain* prev_link;
};

struct qxc_memory_pool* qxc_memory_pool_init(size_t arena_size_bytes);
void qxc_memory_pool_release(struct qxc_memory_pool* pool);
void allocate_arena_chain_link(struct qxc_memory_pool* pool);
char* qxc_malloc_str(struct qxc_memory_pool* pool, size_t bytes);

template <typename T>
T* qxc_malloc(struct qxc_memory_pool* pool)
{
    struct qxc_memory_arena_chain* tip = pool->chain_tip;

    // TODO: exit condition for failure
    while (1) {
        if (tip->bump_ptr + sizeof(T) < tip->end) {
            T* reserved_memory = (T*)(pool->chain_tip->bump_ptr);
            tip->bump_ptr += sizeof(T);
            return reserved_memory;
        }

        allocate_arena_chain_link(pool);
        tip = pool->chain_tip;
    }
}

