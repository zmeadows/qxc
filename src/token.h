#pragma once

#include <stdbool.h>
#include <stddef.h>

enum class Keyword { Return, Int, If, Else, Invalid };

const char* keyword_to_str(Keyword keyword);
Keyword str_to_keyword(const char* kstr);

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

const char* operator_to_str(Operator op);
bool operator_can_be_unary(Operator op);
bool operator_is_always_unary(Operator op);

enum class TokenType {
    CloseBrace,
    CloseParen,
    Identifier,
    IntLiteral,
    KeyWord,
    OpenBrace,
    OpenParen,
    Operator,
    SemiColon,
    Invalid
};

struct Token {
    TokenType type;
    int line;
    int column;

    union {
        char name[256];  // TODO use const char* and qxc_malloc
        long int_literal_value;
        Keyword keyword;
        Operator op;
    };
};

void token_print(const Token& token);

