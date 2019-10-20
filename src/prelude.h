#pragma once

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

#define QXC_FATAL_ERROR(msg)                                       \
    fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, "fatal compiler error:\n%s\n", msg);           \
    exit(EXIT_FAILURE);

#define QXC_UNREACHABLE() QXC_FATAL_ERROR("Supposedly unreachable path encountered")

#define qxc_snprintf(buf, buf_size, ...)                     \
    if (snprintf(buf, buf_size, __VA_ARGS__) > buf_size) {   \
        QXC_FATAL_ERROR("Buffer overflow in string buffer"); \
    }

bool strs_are_equal(const char* strA, const char* strB);

void strip_ext(char* fname);

const char* mk_tmp_dir(void);
void rm_tmp_dir(const char* tmp_path);

size_t max(size_t a, size_t b);

void print_file(const char* filepath);

void qxc_compiler_error_halt(const char* msg);

// struct qxc_small_str {
//     char storage[16];
//     char* contents;
//     size_t capacity;
//     size_t length;
// };
//
// static uint64_t qxc_hash_str(const char* str)
// {
//     uint64_t hash = 5381;
//     unsigned char c;
//
//     while ((c = (unsigned char)*str++)) hash = ((hash << 5) + hash) + c;
//
//     return hash;
// }
