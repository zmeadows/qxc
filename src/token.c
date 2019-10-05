#include "token.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "darray.h"
#include "prelude.h"

const char* qxc_keyword_to_str(enum qxc_keyword keyword)
{
    switch (keyword) {
        case RETURN_KEYWORD:
            return "return";
        case INT_KEYWORD:
            return "int";
        default:
            return NULL;
    }
}

enum qxc_keyword qxc_str_to_keyword(const char* kstr)
{
    if (strs_are_equal("return", kstr)) {
        return RETURN_KEYWORD;
    }
    else if (strs_are_equal("int", kstr)) {
        return INT_KEYWORD;
    }
    else {
        return INVALID_KEYWORD;
    }
}

const char* qxc_operator_to_str(enum qxc_operator op)
{
    switch (op) {
        case MINUS_OP:
            return "-";
        case PLUS_OP:
            return "+";
        case DIVIDE_OP:
            return "/";
        case MULTIPLY_OP:
            return "*";
        case NEGATION_OP:
            return "!";
        case COMPLEMENT_OP:
            return "~";
        case LOGICAL_AND_OP:
            return "&&";
        case LOGICAL_OR_OP:
            return "||";
        case EQUAL_TO_OP:
            return "==";
        case NOT_EQUAL_TO_OP:
            return "!=";
        case LESS_THAN_OP:
            return "<";
        case LESS_THAN_OR_EQUAL_TO_OP:
            return "<=";
        case GREATER_THAN_OP:
            return ">";
        case GREATER_THAN_OR_EQUAL_TO_OP:
            return ">=";
        default:
            return NULL;
    }
}

// TODO: eventually multiply_op will serve as
// unary pointer dereference operator as well
bool qxc_operator_can_be_unary(enum qxc_operator op)
{
    switch (op) {
        case MINUS_OP:
            return true;
        case NEGATION_OP:
            return true;
        case COMPLEMENT_OP:
            return true;
        default:
            return false;
    }
}

bool qxc_operator_is_always_unary(enum qxc_operator op)
{
    switch (op) {
        case NEGATION_OP:
            return true;
        case COMPLEMENT_OP:
            return true;
        default:
            return false;
    }
}

void qxc_token_print(struct qxc_token* token)
{
    printf("%d:%d ", token->line, token->column);

    switch (token->type) {
        case OPEN_BRACE_TOKEN:
            printf("{");
            break;
        case CLOSE_BRACE_TOKEN:
            printf("}");
            break;
        case OPEN_PAREN_TOKEN:
            printf("(");
            break;
        case CLOSE_PAREN_TOKEN:
            printf(")");
            break;
        case SEMICOLON_TOKEN:
            printf(";");
            break;
        case ASSIGNMENT_TOKEN:
            printf("=");
            break;
        case KEYWORD_TOKEN:
            printf("keyword: %s", qxc_keyword_to_str(token->keyword));
            break;
        case IDENTIFIER_TOKEN:
            printf("identifier: %s", token->name);
            break;
        case INTEGER_LITERAL_TOKEN:
            printf("integer literal: %ld", token->int_literal_value);
            break;
        case OPERATOR_TOKEN:
            printf("operator: %s", qxc_operator_to_str(token->op));
            break;
        default:
            printf("unrecognized/invalid token type");
            break;
    }

    printf("\n");
}

IMPLEMENT_DYNAMIC_ARRAY_NEWTYPE(qxc_token_array, struct qxc_token)

