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

// TODO: conver to const char* since eventually we'll have operators with >1 character
char qxc_operator_to_char(enum qxc_operator op)
{
    switch (op) {
        case qxc_minus_op:
            return '-';
        case qxc_plus_op:
            return '+';
        case qxc_divide_op:
            return '/';
        case qxc_multiply_op:
            return '*';
        case qxc_exclamation_op:
            return '!';
        case qxc_complement_op:
            return '~';
        default:
            return '\0';
    }
}

enum qxc_operator char_to_qxc_operator(char opch)
{
    switch (opch) {
        case '-':
            return qxc_minus_op;
        case '+':
            return qxc_plus_op;
        case '/':
            return qxc_divide_op;
        case '*':
            return qxc_multiply_op;
        case '!':
            return qxc_exclamation_op;
        case '~':
            return qxc_complement_op;
        default:
            return qxc_invalid_op;
    }
}

// TODO: eventually multiply_op will serve as unary pointer dereference operator as well
bool qxc_operator_can_be_unary(enum qxc_operator op)
{
    switch (op) {
        case qxc_minus_op:
            return true;
        case qxc_exclamation_op:
            return true;
        case qxc_complement_op:
            return true;
        default:
            return false;
    }
}

bool qxc_operator_is_always_unary(enum qxc_operator op)
{
    switch (op) {
        case qxc_exclamation_op:
            return true;
        case qxc_complement_op:
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
            printf("operator: %c", qxc_operator_to_char(token->op));
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
    new_token->op = qxc_invalid_op;

    buffer->length++;
    return new_token;
}
