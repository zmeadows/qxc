#pragma once

#include <stddef.h>
#include <stdint.h>

struct qxc_memory_pool;
struct qxc_memory_pool* qxc_memory_pool_init(size_t arena_size_bytes);
void qxc_memory_pool_release(struct qxc_memory_pool* pool);
void* qxc_malloc(struct qxc_memory_pool* pool, size_t bytes);
