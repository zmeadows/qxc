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
        return qxc_not_a_keyword;
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
            printf("keyword: %s", token->name);
            break;
        case qxc_identifier_token:
            printf("identifier: %s", token->name);
            break;
        case qxc_integer_literal_token:
            printf("integer literal: %d", token->value);
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
    new_token->type = qxc_invalid_token_type;
    new_token->line = -1;
    new_token->column = -1;
    new_token->name[0] = '\0';
    new_token->value = 0;
    buffer->length++;
    return new_token;
}
