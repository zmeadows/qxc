#include "allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// FIXME: what if user requests more than QXC_MEMORY_POOL_SIZE_BYTES with qxc_malloc?
#define QXC_MEMORY_POOL_SIZE_BYTES ((size_t)(500e3))

struct qxc_memory_pool {
    uint8_t* start;
    uint8_t* end;
    uint8_t* ptr;
    struct qxc_memory_pool* next_pool;
};

static struct qxc_memory_pool* s_pool = NULL;

static void qxc_memory_release(void)
{
    struct qxc_memory_pool* pool = s_pool;

    while (pool) {
        free(pool->start);
        pool = pool->next_pool;
    }
}

static struct qxc_memory_pool* qxc_memory_pool_init(void)
{
    struct qxc_memory_pool* new_pool = malloc(sizeof(struct qxc_memory_pool));

    new_pool->start = calloc(QXC_MEMORY_POOL_SIZE_BYTES, 1);

    if (new_pool->start == NULL) {
        free(new_pool);
        fprintf(stderr, "Failed to allocate memory pool for qxc! Exiting...\n");
        exit(EXIT_FAILURE);
        return NULL;
    }

    new_pool->end = new_pool->start + QXC_MEMORY_POOL_SIZE_BYTES;
    new_pool->ptr = new_pool->start;
    new_pool->next_pool = NULL;

    return new_pool;
}

void* qxc_malloc(size_t bytes)
{
    if (s_pool == NULL) {
        s_pool = qxc_memory_pool_init();
        atexit(qxc_memory_release);
    }

    struct qxc_memory_pool* pool = s_pool;

    while (1) {
        if (pool->ptr < pool->end - bytes) {
            void* req = (void*)pool->ptr;
            pool->ptr += bytes;
            return req;
        }
        else if (!pool->next_pool) {
            pool->next_pool = qxc_memory_pool_init();
        }

        pool = pool->next_pool;
    }
}

