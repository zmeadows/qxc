#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prelude.h"
#include "token.h"

#define QXC_MAXIMUM_IDENTIFIER_LENGTH 256

struct qxc_tokenizer {
    char id[QXC_MAXIMUM_IDENTIFIER_LENGTH];
    size_t id_len;

    char* contents;
    int current_line;
    int current_column;

    char* next_char_ptr;
    char next_char;
};

static void qxc_tokenizer_advance(struct qxc_tokenizer* tokenizer)
{
    tokenizer->next_char_ptr++;
    tokenizer->next_char = *tokenizer->next_char_ptr;
    tokenizer->current_column++;
}

static void qxc_tokenizer_init(struct qxc_tokenizer* tokenizer, const char* filepath)
{
    FILE* f = fopen(filepath, "r");

    // get the file size in bytes
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    tokenizer->contents = malloc(fsize + 1);
    fread(tokenizer->contents, 1, fsize, f);
    fclose(f);

    tokenizer->id[0] = '\0';
    tokenizer->id_len = 0;

    tokenizer->current_line = 1;
    tokenizer->current_column = 1;

    tokenizer->next_char_ptr = tokenizer->contents;
    tokenizer->next_char = *tokenizer->next_char_ptr;
}

// TODO remove?
static void qxc_tokenizer_grow_id_buffer(struct qxc_tokenizer* tokenizer)
{
    tokenizer->id[tokenizer->id_len] = tokenizer->next_char;
    tokenizer->id_len++;
}

static bool is_valid_keyword_identifier_first_character(char c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'));
}

static bool is_valid_keyword_identifier_trailing_character(char c)
{
    return is_valid_keyword_identifier_first_character(c) || (c >= '0' && c <= '9');
}

// FIXME: need separate functions for consuming different types of
// literals/identifiers
static void qxc_tokenizer_consume_id(struct qxc_tokenizer* tokenizer)
{
    tokenizer->id[0] = '\0';
    tokenizer->id_len = 0;
    qxc_tokenizer_grow_id_buffer(tokenizer);
    qxc_tokenizer_advance(tokenizer);

    while (is_valid_keyword_identifier_trailing_character(tokenizer->next_char)) {
        // if (tokenizer.id_len == QXC_MAXIMUM_IDENTIFIER_LENGTH-1) {  // report
        // error
        // }
        qxc_tokenizer_grow_id_buffer(tokenizer);
        qxc_tokenizer_advance(tokenizer);
    }

    tokenizer->id[tokenizer->id_len] = '\0';
}

static void qxc_consume_single_char_token(struct qxc_tokenizer* tokenizer,
                                          struct qxc_token_buffer* token_buffer, enum qxc_token_type type)
{
    struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);
    new_token->type = type;
    new_token->line = tokenizer->current_line;
    new_token->column = tokenizer->current_column;
    qxc_tokenizer_advance(tokenizer);
}

static void qxc_consume_unary_op_token(struct qxc_tokenizer* tokenizer, struct qxc_token_buffer* token_buffer,
                                       enum qxc_unary_op op)
{
    struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);
    new_token->type = qxc_unary_op_token;
    new_token->unary_op = op;
    new_token->line = tokenizer->current_line;
    new_token->column = tokenizer->current_column;
    qxc_tokenizer_advance(tokenizer);
}

struct qxc_token_buffer* qxc_tokenize(const char* filepath)
{
    debug_print("starting lexer for file: %s\n", filepath);

    struct qxc_tokenizer tokenizer;
    qxc_tokenizer_init(&tokenizer, filepath);

    struct qxc_token_buffer* token_buffer = qxc_token_buffer_alloc();

    while (1) {
        if (is_valid_keyword_identifier_first_character(tokenizer.next_char)) {
            qxc_tokenizer_consume_id(&tokenizer);

            struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);

            enum qxc_keyword keyword = qxc_str_to_keyword(tokenizer.id);

            if (keyword == qxc_not_a_keyword) {  // it's an identifier
                new_token->type = qxc_identifier_token;

                // TODO: bounds check identifier length
                memcpy(new_token->name, tokenizer.id, tokenizer.id_len + 1);
            }
            else {  // it's a built in keyword
                new_token->type = qxc_keyword_token;
                new_token->keyword = keyword;
            }

            new_token->line = tokenizer.current_line;
            new_token->column = tokenizer.current_column - tokenizer.id_len;
        }

        else if (tokenizer.next_char == '{') {
            qxc_consume_single_char_token(&tokenizer, token_buffer, qxc_open_brace_token);
        }

        else if (tokenizer.next_char == '}') {
            qxc_consume_single_char_token(&tokenizer, token_buffer, qxc_close_brace_token);
        }

        else if (tokenizer.next_char == '(') {
            qxc_consume_single_char_token(&tokenizer, token_buffer, qxc_open_paren_token);
        }

        else if (tokenizer.next_char == ')') {
            qxc_consume_single_char_token(&tokenizer, token_buffer, qxc_close_paren_token);
        }

        else if (tokenizer.next_char == ';') {
            qxc_consume_single_char_token(&tokenizer, token_buffer, qxc_semicolon_token);
        }

        else if (tokenizer.next_char == '-') {
            qxc_consume_unary_op_token(&tokenizer, token_buffer, qxc_negation_unary_op);
        }

        else if (tokenizer.next_char == '!') {
            qxc_consume_unary_op_token(&tokenizer, token_buffer, qxc_logical_negation_unary_op);
        }

        else if (tokenizer.next_char == '~') {
            qxc_consume_unary_op_token(&tokenizer, token_buffer, qxc_bitwise_complement_unary_op);
        }

        else if (tokenizer.next_char == '\n') {
            tokenizer.next_char_ptr++;
            tokenizer.next_char = *tokenizer.next_char_ptr;
            tokenizer.current_line++;
            tokenizer.current_column = 1;
        }
        else if (tokenizer.next_char == ' ') {
            qxc_tokenizer_advance(&tokenizer);
        }

        else if (tokenizer.next_char >= '0' && tokenizer.next_char <= '9') {
            qxc_tokenizer_consume_id(&tokenizer);

            errno = 0;
            int maybe_value = strtol(tokenizer.id, NULL, 10);
            if (errno != 0) {
                printf("invalid numeric literal: %s\n", tokenizer.id);
                exit(1);
            }

            struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);
            new_token->type = qxc_integer_literal_token;
            new_token->line = tokenizer.current_line;
            new_token->column = tokenizer.current_column - tokenizer.id_len;
            new_token->value = maybe_value;

            // qxc_tokenizer_advance(&tokenizer);
        }

        else if (tokenizer.next_char == '\0') {
            // we're done!
            break;
        }

        else {
            debug_print("encountered unexpected character: %c %d\n", tokenizer.next_char,
                        (int)tokenizer.next_char);
            // TODO: free buffer and return NULL here
            break;
        }
    }

    debug_print("finished lexer for file: %s\n", filepath);
    free(tokenizer.contents);

    return token_buffer;
}

