#include "allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static inline size_t arena_size_bytes(struct qxc_memory_arena_chain* arena)
{
    assert(arena->end > arena->start);
    return (size_t)((int64_t)arena->end - (int64_t)arena->start);
}

static void allocate_arena_chain_link(struct qxc_memory_pool* pool)
{
    struct qxc_memory_arena_chain* old_tip = pool->chain_tip;

    struct qxc_memory_arena_chain* new_tip =
        malloc(sizeof(struct qxc_memory_arena_chain));
    new_tip->start = malloc(pool->arena_size);
    new_tip->end = new_tip->start + pool->arena_size;
    new_tip->bump_ptr = new_tip->start;
    new_tip->prev_link = old_tip;

    pool->chain_tip = new_tip;
}

struct qxc_memory_pool* qxc_memory_pool_init(size_t arena_size_bytes)
{
    struct qxc_memory_pool* new_pool = malloc(sizeof(struct qxc_memory_pool));
    new_pool->chain_tip = NULL;
    new_pool->arena_size = arena_size_bytes;
    allocate_arena_chain_link(new_pool);
    return new_pool;
}

void* qxc_malloc(struct qxc_memory_pool* pool, size_t bytes)
{
    struct qxc_memory_arena_chain* tip = pool->chain_tip;

    // size_t arena_size = tip->end - tip->start;
    // assert(bytes < arena_size);

    // TODO: exit condition for failure
    while (1) {
        if (tip->bump_ptr + bytes < tip->end) {
            void* reserved_memory = pool->chain_tip->bump_ptr;
            tip->bump_ptr += bytes;
            return reserved_memory;
        }

        allocate_arena_chain_link(pool);
        tip = pool->chain_tip;
    }
}

void qxc_memory_pool_release(struct qxc_memory_pool* pool)
{
    struct qxc_memory_arena_chain* tip = pool->chain_tip;

    while (tip) {
        struct qxc_memory_arena_chain* old_tip = tip;
        tip = tip->prev_link;
        free(old_tip->start);
        free(old_tip);
    }

    free(pool);
}

