#pragma once

#include <stddef.h>

void qxc_memory_reserve(void);
void qxc_memory_release(void);
void* qxc_malloc(size_t bytes);

#define QXC_MALLOC(t)                \
    do {                             \
        (t*)qxc_malloc(sizeof(*(t))) \
    } while (0)
