#pragma once

#define _XOPEN_SOURCE 500L

#include <stdbool.h>
#include <stdlib.h>

// #define DEBUG

#ifdef DEBUG
#define debug_print(...)                                               \
    do {                                                               \
        fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); \
        fprintf(stderr, __VA_ARGS__);                                  \
        fprintf(stderr, "\n");                                         \
    } while (0)
#else
#define debug_print(...) \
    do {                 \
    } while (0)
#endif

bool strs_are_equal(const char* strA, const char* strB);

void strip_ext(char* fname);

const char* mk_tmp_dir(void);
void rm_tmp_dir(const char* tmp_path);

size_t max(size_t a, size_t b);

// struct qxc_small_str {
//     char storage[16];
//     char* contents;
//     size_t capacity;
//     size_t length;
// };
