#pragma once

#include <stdbool.h>

#define DEBUG

#ifdef DEBUG
#define debug_print(...)                                               \
    do {                                                               \
        fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); \
        fprintf(stderr, __VA_ARGS__);                                  \
        fprintf(stderr, "\n");                                         \
    } while (0)
#else
#define debug_print(x) \
    do {               \
    } while (0)
#endif

bool strs_are_equal(const char* strA, const char* strB);

void strip_ext(char* fname);

// enum qxc_error_code { qxc_no_error, qxc_lexer_error, qxc_parser_error, qxc_codegen_error };
//
// struct qxc_error {
//     enum qxc_error_code code;
//     int line;
//     int column;
//     const char* filepath;
//     const char* description;
// };
//
// void qxc_error_reset(struct qxc_error* error);
// void qxc_error_print(struct qxc_error* error);
