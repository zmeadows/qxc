#include "allocator.h"

#include <assert.h>
#include <stdlib.h>

#define QXC_MEMORY_POOL_SIZE_BYTES (size_t)1e6

struct qxc_memory_pool {
    uint8_t* start;
    uint8_t* end;
    uint8_t* ptr;
    struct qxc_memory_pool* next_pool;
};

static struct qxc_memory_pool* qxc_memory_pool_init(void) {
    struct qxc_memory_pool* new_pool = malloc(sizeof(struct qxc_memory_pool));

    new_pool->start = calloc(QXC_MEMORY_POOL_SIZE_BYTES, 1);
    new_pool->end = new_pool->start + QXC_MEMORY_POOL_SIZE_BYTES;
    new_pool->ptr = new_pool->start;
    new_pool->next_pool = NULL;

    return new_pool;
}

static void qxc_memory_pool_release(struct qxc_memory_pool* pool) {
    while (pool->next_pool) {
        qxc_memory_pool_release(pool->next_pool);
    }

    free(pool->start);
}

static struct qxc_memory_pool* s_pool = NULL;

void* qxc_malloc(size_t bytes) {
    assert(s_pool != NULL);
    struct qxc_memory_pool* pool = s_pool;

    while (1) {
        if (pool->ptr < pool->end - bytes) {
            void* req = (void*)pool->ptr;
            pool->ptr += bytes;
            return req;
        } else if (!pool->next_pool) {
            pool->next_pool = qxc_memory_pool_init();
        }

        pool = pool->next_pool;
    }
}

void qxc_memory_reserve(void) { s_pool = qxc_memory_pool_init(); }
void qxc_memory_release(void) { qxc_memory_pool_release(s_pool); }

