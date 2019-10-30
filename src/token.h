#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "darray.h"

enum class Keyword { Return, Int, If, Else, Invalid };

const char* qxc_keyword_to_str(Keyword keyword);
Keyword qxc_str_to_keyword(const char* kstr);

enum class Operator {
    Minus,
    Plus,
    Divide,
    Multiply,
    LogicalNegation,
    Complement,
    LogicalAND,
    LogicalOR,
    EqualTo,
    NotEqualTo,
    Colon,
    QuestionMark,
    LessThan,
    LessThanOrEqualTo,
    GreaterThan,
    GreaterThanOrEqualTo,
    Assignment,
    Comma,
    BitwiseOR,
    BitwiseAND,
    BitwiseXOR,
    BitShiftLeft,
    BitShiftRight,
    Percent,
    Invalid
};

const char* qxc_operator_to_str(Operator op);
// Operator str_to_qxc_operator(const char* opstr);
bool qxc_operator_can_be_unary(Operator op);
bool qxc_operator_is_always_unary(Operator op);

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
        Keyword keyword;
        Operator op;
    };
};

void qxc_token_print(struct qxc_token* token);

