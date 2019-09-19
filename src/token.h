#pragma once

#include <stddef.h>

enum qxc_token_type {
    qxc_close_brace_token,
    qxc_close_paren_token,
    qxc_identifier_token,
    qxc_integer_literal_token,
    qxc_keyword_token,
    qxc_open_brace_token,
    qxc_open_paren_token,
    qxc_semicolon_token,
    qxc_unary_op_token,
    qxc_num_token_types,
    qxc_invalid_token_type
};

enum qxc_keyword { qxc_return_keyword, qxc_int_keyword, qxc_num_keywords, qxc_not_a_keyword };

enum qxc_unary_op {
    qxc_negation_unary_op,
    qxc_logical_negation_unary_op,
    qxc_bitwise_complement_unary_op,
    qxc_num_unary_ops,
    qxc_not_a_unary_op
};

char qxc_unary_op_to_char(enum qxc_unary_op op);
enum qxc_unary_op qxc_char_to_unary_op(char opch);

struct qxc_token {
    enum qxc_token_type type;
    int line;
    int column;

    union {
        char name[256];
        int int_literal_value;
        enum qxc_keyword keyword;
        enum qxc_unary_op unary_op;
    };
};

const char* qxc_keyword_to_str(enum qxc_keyword keyword);
enum qxc_keyword qxc_str_to_keyword(const char* kstr);

void qxc_token_print(struct qxc_token* token);

struct qxc_token_buffer {
    struct qxc_token* tokens;
    size_t capacity;
    size_t length;
};

struct qxc_token_buffer* qxc_token_buffer_alloc(void);
void qxc_token_buffer_free(struct qxc_token_buffer* buffer);
struct qxc_token* qxc_token_buffer_extend(struct qxc_token_buffer* buffer);
