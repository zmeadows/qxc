#pragma once

#include "darray.h"

#include <stdbool.h>
#include <stddef.h>

enum qxc_keyword {
    RETURN_KEYWORD,
    INT_KEYWORD,
    IF_KEYWORD,
    ELSE_KEYWORD,
    INVALID_KEYWORD
};

const char* qxc_keyword_to_str(enum qxc_keyword keyword);
enum qxc_keyword qxc_str_to_keyword(const char* kstr);

enum qxc_operator {
    MINUS_OP,
    PLUS_OP,
    DIVIDE_OP,
    MULTIPLY_OP,
    NEGATION_OP,
    COMPLEMENT_OP,
    LOGICAL_AND_OP,
    LOGICAL_OR_OP,
    EQUAL_TO_OP,
    NOT_EQUAL_TO_OP,
    COLON_OP,
    QUESTION_MARK_OP,
    LESS_THAN_OP,
    LESS_THAN_OR_EQUAL_TO_OP,
    GREATER_THAN_OP,
    GREATER_THAN_OR_EQUAL_TO_OP,
    ASSIGNMENT_OP,
    COMMA_OP,
    BITWISE_OR_OP,
    BITWISE_AND_OP,
    BITWISE_XOR_OP,
    BITSHIFT_LEFT_OP,
    BITSHIFT_RIGHT_OP,
    PERCENT_OP,
    INVALID_OP
};

const char* qxc_operator_to_str(enum qxc_operator op);
// enum qxc_operator str_to_qxc_operator(const char* opstr);
bool qxc_operator_can_be_unary(enum qxc_operator op);
bool qxc_operator_is_always_unary(enum qxc_operator op);

enum qxc_token_type {
    CLOSE_BRACE_TOKEN,
    CLOSE_PAREN_TOKEN,
    IDENTIFIER_TOKEN,
    INTEGER_LITERAL_TOKEN,
    KEYWORD_TOKEN,
    OPEN_BRACE_TOKEN,
    OPEN_PAREN_TOKEN,
    OPERATOR_TOKEN,
    SEMICOLON_TOKEN,
    INVALID_TOKEN
};

struct qxc_token {
    enum qxc_token_type type;
    int line;
    int column;

    union {
        char name[256];  // TODO use const char* and qxc_malloc
        long int_literal_value;
        enum qxc_keyword keyword;
        enum qxc_operator op;
    };
};

void qxc_token_print(struct qxc_token* token);

DECLARE_DYNAMIC_ARRAY_NEWTYPE(qxc_token_array, struct qxc_token)

