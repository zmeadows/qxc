#include "token.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "darray.h"
#include "prelude.h"

const char* keyword_to_str(Keyword keyword)
{
    switch (keyword) {
        case Keyword::Return:
            return "return";
        case Keyword::Int:
            return "int";
        case Keyword::If:
            return "if";
        case Keyword::Else:
            return "else";
        default:
            return nullptr;
    }
}

Keyword str_to_keyword(const char* kstr)
{
    if (strs_are_equal("return", kstr)) {
        return Keyword::Return;
    }
    else if (strs_are_equal("int", kstr)) {
        return Keyword::Int;
    }
    else if (strs_are_equal("if", kstr)) {
        return Keyword::If;
    }
    else if (strs_are_equal("else", kstr)) {
        return Keyword::Else;
    }
    else {
        return Keyword::Invalid;
    }
}

const char* operator_to_str(Operator op)
{
    switch (op) {
        case Operator::Minus:
            return "-";
        case Operator::Plus:
            return "+";
        case Operator::Divide:
            return "/";
        case Operator::Multiply:
            return "*";
        case Operator::LogicalNegation:
            return "!";
        case Operator::Complement:
            return "~";
        case Operator::LogicalAND:
            return "&&";
        case Operator::LogicalOR:
            return "||";
        case Operator::EqualTo:
            return "==";
        case Operator::NotEqualTo:
            return "!=";
        case Operator::LessThan:
            return "<";
        case Operator::LessThanOrEqualTo:
            return "<=";
        case Operator::GreaterThan:
            return ">";
        case Operator::GreaterThanOrEqualTo:
            return ">=";
        case Operator::Colon:
            return ":";
        case Operator::QuestionMark:
            return "?";
        case Operator::Assignment:
            return "=";
        default:
            return nullptr;
    }
}

// TODO: eventually multiply_op will serve as
// unary pointer dereference operator as well
bool operator_can_be_unary(Operator op)
{
    switch (op) {
        case Operator::Minus:
            return true;
        case Operator::LogicalNegation:
            return true;
        case Operator::Complement:
            return true;
        default:
            return false;
    }
}

bool operator_is_always_unary(Operator op)
{
    switch (op) {
        case Operator::LogicalNegation:
            return true;
        case Operator::Complement:
            return true;
        default:
            return false;
    }
}

void token_print(const Token& token)
{
    printf("%d:%d ", token.line, token.column);

    switch (token.type) {
        case TokenType::OpenBrace:
            printf("{");
            break;
        case TokenType::CloseBrace:
            printf("}");
            break;
        case TokenType::OpenParen:
            printf("(");
            break;
        case TokenType::CloseParen:
            printf(")");
            break;
        case TokenType::SemiColon:
            printf(";");
            break;
        case TokenType::KeyWord:
            printf("keyword: %s", keyword_to_str(token.keyword));
            break;
        case TokenType::Identifier:
            printf("identifier: %s", token.name);
            break;
        case TokenType::IntLiteral:
            printf("integer literal: %ld", token.int_literal_value);
            break;
        case TokenType::Operator:
            printf("operator: %s", operator_to_str(token.op));
            break;
        default:
            printf("unrecognized/invalid token type");
            break;
    }

    printf("\n");
}

