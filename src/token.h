#pragma once

#include <stdbool.h>
#include <stddef.h>

enum qxc_keyword {
    qxc_return_keyword,
    qxc_int_keyword,
    qxc_keyword_count,
    qxc_invalid_keyword
};

const char* qxc_keyword_to_str(enum qxc_keyword keyword);
enum qxc_keyword qxc_str_to_keyword(const char* kstr);

enum qxc_operator {
    qxc_minus_op,
    qxc_plus_op,
    qxc_divide_op,
    qxc_multiply_op,
    qxc_exclamation_op,
    qxc_complement_op,
    qxc_operator_count,
    qxc_invalid_op
};

char qxc_operator_to_char(enum qxc_operator op);
enum qxc_operator char_to_qxc_operator(char opch);
bool qxc_operator_can_be_unary(enum qxc_operator op);
bool qxc_operator_is_always_unary(enum qxc_operator op);

// TODO: symbol token
enum qxc_token_type {
    qxc_close_brace_token,
    qxc_close_paren_token,
    qxc_identifier_token,
    qxc_integer_literal_token,
    qxc_keyword_token,
    qxc_open_brace_token,
    qxc_open_paren_token,
    qxc_operator_token,
    qxc_semicolon_token,
    qxc_token_type_count,
    qxc_invalid_token
};

struct qxc_token {
    enum qxc_token_type type;
    int line;
    int column;

    union {
        char name[256];  // TODO use const char* and qxc_malloc
        int int_literal_value;
        enum qxc_keyword keyword;
        enum qxc_operator op;
    };
};

void qxc_token_print(struct qxc_token* token);

struct qxc_token_buffer {
    struct qxc_token* tokens;
    size_t capacity;
    size_t length;
};

struct qxc_token_buffer* qxc_token_buffer_alloc(void);
void qxc_token_buffer_free(struct qxc_token_buffer* buffer);
struct qxc_token* qxc_token_buffer_extend(struct qxc_token_buffer* buffer);
