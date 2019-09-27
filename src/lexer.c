#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prelude.h"
#include "token.h"

#define QXC_MAXIMUM_IDENTIFIER_LENGTH 256

// TODO: convert tokenizer/token_buffer to use qxc_malloc

struct qxc_tokenizer {
    // TODO: store identifiers in a hash set/table and use one pointer per identifier
    // maybe table of const char* -> list of all locations of identifier/keyword?
    char id[QXC_MAXIMUM_IDENTIFIER_LENGTH];
    size_t id_len;

    char* contents;
    int current_line;
    int current_column;

    char* next_char_ptr;
    char next_char;
};

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

    printf("\n\n");
    printf("%s\n", tokenizer->contents);
    printf("\n\n");

    tokenizer->id[0] = '\0';
    tokenizer->id_len = 0;

    tokenizer->current_line = 1;
    tokenizer->current_column = 1;

    tokenizer->next_char_ptr = tokenizer->contents;
    tokenizer->next_char = *tokenizer->next_char_ptr;
}

static void qxc_tokenizer_advance(struct qxc_tokenizer* tokenizer)
{
    tokenizer->next_char_ptr++;
    tokenizer->next_char = *tokenizer->next_char_ptr;
    tokenizer->current_column++;
}

static void qxc_tokenizer_free(struct qxc_tokenizer* tokenizer)
{
    free(tokenizer->contents);
}

// TODO remove?
static inline void qxc_tokenizer_grow_id_buffer(struct qxc_tokenizer* tokenizer)
{
    tokenizer->id[tokenizer->id_len] = tokenizer->next_char;
    tokenizer->id_len++;
}

static inline bool is_valid_keyword_identifier_first_character(char c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'));
}

static inline bool is_valid_operator_first_character(char c)
{
    return c != '\0' && strchr("-+/*!~&|=<>", c) != NULL;
}

static inline bool can_be_digraph_first_character(char c) { return strchr("&|!=<>", c); }

static inline enum qxc_operator try_build_digraph_operator(char c1, char c2)
{
    if (!can_be_digraph_first_character(c1)) return INVALID_OP;

    switch (c1) {
        case '&':
            if (c2 == '&') return LOGICAL_AND_OP;
            break;
        case '|':
            if (c2 == '|') return LOGICAL_OR_OP;
            break;
        case '!':
            if (c2 == '=') return NOT_EQUAL_TO_OP;
            break;
        case '=':
            if (c2 == '=') return EQUAL_TO_OP;
            break;
        case '<':
            if (c2 == '=') return LESS_THAN_OR_EQUAL_TO_OP;
            break;
        case '>':
            if (c2 == '=') return GREATER_THAN_OR_EQUAL_TO_OP;
            break;
    }

    return INVALID_OP;
}

static inline enum qxc_operator build_unigraph_operator(char c)
{
    switch (c) {
        case '-':
            return MINUS_OP;
        case '+':
            return PLUS_OP;
        case '/':
            return DIVIDE_OP;
        case '*':
            return MULTIPLY_OP;
        case '!':
            return EXCLAMATION_OP;
        case '>':
            return GREATER_THAN_OP;
        case '<':
            return LESS_THAN_OP;
        case '~':
            return COMPLEMENT_OP;
        default:
            return INVALID_OP;
    }
}

static inline bool is_valid_keyword_identifier_trailing_character(char c)
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

static inline bool is_valid_symbol(char c)
{
    return c != '\0' && strchr("{}();", c) != NULL;
}

static void qxc_consume_symbol_token(struct qxc_tokenizer* tokenizer,
                                     struct qxc_token_buffer* token_buffer)
{
    struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);
    new_token->line = tokenizer->current_line;
    new_token->column = tokenizer->current_column;
    switch (tokenizer->next_char) {
        case '{':
            new_token->type = qxc_open_brace_token;
            break;
        case '}':
            new_token->type = qxc_close_brace_token;
            break;
        case '(':
            new_token->type = qxc_open_paren_token;
            break;
        case ')':
            new_token->type = qxc_close_paren_token;
            break;
        case ';':
            new_token->type = qxc_semicolon_token;
            break;
        default:
            new_token->type = qxc_invalid_token;
            break;
    }
    qxc_tokenizer_advance(tokenizer);
}

static void qxc_build_operator_token(struct qxc_tokenizer* tokenizer,
                                     struct qxc_token_buffer* token_buffer,
                                     enum qxc_operator op)
{
    struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);
    new_token->type = qxc_operator_token;
    new_token->op = op;
    new_token->line = tokenizer->current_line;
    new_token->column = tokenizer->current_column;
}

// 'consume' == build + advance tokenizer
static void qxc_consume_operator_token(struct qxc_tokenizer* tokenizer,
                                       struct qxc_token_buffer* token_buffer,
                                       enum qxc_operator op)
{
    qxc_build_operator_token(tokenizer, token_buffer, op);
    qxc_tokenizer_advance(tokenizer);
}

struct qxc_token_buffer* qxc_tokenize(const char* filepath)
{
    struct qxc_tokenizer tokenizer;
    qxc_tokenizer_init(&tokenizer, filepath);

    struct qxc_token_buffer* token_buffer = qxc_token_buffer_alloc();

    while (1) {
        if (is_valid_keyword_identifier_first_character(tokenizer.next_char)) {
            qxc_tokenizer_consume_id(&tokenizer);

            struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);

            enum qxc_keyword keyword = qxc_str_to_keyword(tokenizer.id);

            if (keyword == qxc_invalid_keyword) {
                // it's an identifier, not a keyword
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

        // TODO: use switch statement here
        else if (is_valid_symbol(tokenizer.next_char)) {
            qxc_consume_symbol_token(&tokenizer, token_buffer);
        }

        else if (is_valid_operator_first_character(tokenizer.next_char)) {
            char c1 = tokenizer.next_char;
            qxc_tokenizer_advance(&tokenizer);
            char c2 = tokenizer.next_char;

            enum qxc_operator op = try_build_digraph_operator(c1, c2);

            if (op != INVALID_OP) {
                qxc_consume_operator_token(&tokenizer, token_buffer, op);
            }
            else {
                qxc_build_operator_token(&tokenizer, token_buffer,
                                         build_unigraph_operator(c1));
            }
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
                // err->code = qxc_lexer_error;
                // err->line = tokenizer.current_line;
                // err->column = tokenizer.current_column - tokenizer.id_len;
                // err->filepath = filepath;
                // err->description = "Failed to parse integer literal";
                fprintf(stderr, "failed to parse integer literal\n");
                goto failure;
            }

            struct qxc_token* new_token = qxc_token_buffer_extend(token_buffer);
            new_token->type = qxc_integer_literal_token;
            new_token->line = tokenizer.current_line;
            new_token->column = tokenizer.current_column - tokenizer.id_len;
            new_token->int_literal_value = maybe_value;

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

    qxc_tokenizer_free(&tokenizer);
    return token_buffer;

failure:
    qxc_tokenizer_free(&tokenizer);
    qxc_token_buffer_free(token_buffer);
    return NULL;
}

