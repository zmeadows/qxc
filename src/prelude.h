#pragma once

#include <stdbool.h>

#define DEBUG

#ifdef DEBUG
#define debug_print(fmt, ...)                                             \
    do {                                                                  \
        fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
                __VA_ARGS__);                                             \
    } while (0)
#else
#define debug_print(x) \
    do {               \
    } while (0)
#endif

bool strs_are_equal(const char* strA, const char* strB);
