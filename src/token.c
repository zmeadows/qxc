#include "token.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "prelude.h"

static inline size_t max(size_t a, size_t b) { return a > b ? a : b; }

const char* qxc_keyword_to_str(enum qxc_keyword keyword)
{
    switch (keyword) {
        case qxc_return_keyword:
            return "return";
        case qxc_int_keyword:
            return "int";
        default:
            return NULL;
    }
}

enum qxc_keyword qxc_str_to_keyword(const char* kstr)
{
    if (strs_are_equal("return", kstr)) {
        return qxc_return_keyword;
    }
    else if (strs_are_equal("int", kstr)) {
        return qxc_int_keyword;
    }
    else {
        return qxc_invalid_keyword;
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
        case EXCLAMATION_OP:
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

// enum qxc_operator str_to_qxc_operator(const char* opstr)
// {
//     if (strs_are_equal("-", opstr)) {
//         return MINUS_OP;
//     }
//     else if (strs_are_equal("+", opstr)) {
//         return PLUS_OP;
//     }
//     else if (strs_are_equal("/", opstr)) {
//         return DIVIDE_OP;
//     }
// }

// TODO: eventually multiply_op will serve as unary pointer dereference operator as well
bool qxc_operator_can_be_unary(enum qxc_operator op)
{
    switch (op) {
        case MINUS_OP:
            return true;
        case EXCLAMATION_OP:
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
        case EXCLAMATION_OP:
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
        case qxc_open_brace_token:
            printf("{");
            break;
        case qxc_close_brace_token:
            printf("}");
            break;
        case qxc_open_paren_token:
            printf("(");
            break;
        case qxc_close_paren_token:
            printf(")");
            break;
        case qxc_semicolon_token:
            printf(";");
            break;
        case qxc_keyword_token:
            printf("keyword: %s", qxc_keyword_to_str(token->keyword));
            break;
        case qxc_identifier_token:
            printf("identifier: %s", token->name);
            break;
        case qxc_integer_literal_token:
            printf("integer literal: %d", token->int_literal_value);
            break;
        case qxc_operator_token:
            printf("operator: %s", qxc_operator_to_str(token->op));
            break;
        default:
            printf("unrecognized/invalid token type");
            break;
    }

    printf("\n");
}

struct qxc_token_buffer* qxc_token_buffer_alloc(void)
{
    struct qxc_token_buffer* token_buffer = malloc(sizeof(struct qxc_token_buffer));
    token_buffer->tokens = NULL;
    token_buffer->capacity = 0;
    token_buffer->length = 0;
    return token_buffer;
}

void qxc_token_buffer_free(struct qxc_token_buffer* buffer) { free(buffer->tokens); }

struct qxc_token* qxc_token_buffer_extend(struct qxc_token_buffer* buffer)
{
    if (buffer->capacity == buffer->length) {
        size_t new_capacity = max(32, ceil(1.61803398875 * buffer->capacity));
        buffer->tokens = realloc(buffer->tokens, new_capacity);
        buffer->capacity = new_capacity;
    }

    struct qxc_token* new_token = &buffer->tokens[buffer->length];
    new_token->type = qxc_invalid_token;
    new_token->line = -1;
    new_token->column = -1;
    new_token->name[0] = '\0';
    new_token->int_literal_value = 0;
    new_token->keyword = qxc_invalid_keyword;
    new_token->op = INVALID_OP;

    buffer->length++;
    return new_token;
}

