#include "lexer.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "array.h"
#include "prelude.h"
#include "token.h"

#define QXC_MAXIMUM_IDENTIFIER_LENGTH 256

struct Tokenizer {
    // TODO: store identifiers in a hash set/table and use one pointer per identifier
    // maybe table of const char* -> list of all locations of identifier/keyword?
    char id[QXC_MAXIMUM_IDENTIFIER_LENGTH];
    char* contents;
    char* next_char_ptr;
    int id_len;
    int current_line;
    int current_column;
    char next_char;
};

static int tokenizer_init(Tokenizer* tokenizer, const char* filepath)
{
    FILE* f = fopen(filepath, "r");

    if (f == nullptr) {
        return -1;
    }

    // get the file size in bytes
    fseek(f, 0, SEEK_END);
    long fsizel = ftell(f);

    if (fsizel < 0) {
        fclose(f);
        return -1;
    }

    size_t fsize = (size_t)fsizel;

    rewind(f);

    tokenizer->contents = (char*)malloc(fsize + 1);
    fread(tokenizer->contents, 1, fsize, f);
    fclose(f);

    tokenizer->id[0] = '\0';
    tokenizer->id_len = 0;

    tokenizer->current_line = 1;
    tokenizer->current_column = 1;

    tokenizer->next_char_ptr = tokenizer->contents;
    tokenizer->next_char = *tokenizer->next_char_ptr;

    return 0;
}

static void tokenizer_free(Tokenizer* tokenizer) { free(tokenizer->contents); }

static void tokenizer_advance(Tokenizer* tokenizer)
{
    tokenizer->next_char_ptr++;
    tokenizer->next_char = *tokenizer->next_char_ptr;
    tokenizer->current_column++;
}

// TODO remove?
static inline void tokenizer_grow_id_buffer(Tokenizer* tokenizer)
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
    return c != '\0' && strchr(":?-+/*!~&|=<>", c) != nullptr;
}

static inline bool can_be_digraph_first_character(char c) { return strchr("&|!=<>", c); }

static inline Operator try_build_digraph_operator(char c1, char c2)
{
    if (!can_be_digraph_first_character(c1)) return Operator::Invalid;

    switch (c1) {
        case '&':
            if (c2 == '&') return Operator::LogicalAND;
            break;
        case '|':
            if (c2 == '|') return Operator::LogicalOR;
            break;
        case '!':
            if (c2 == '=') return Operator::NotEqualTo;
            break;
        case '=':
            if (c2 == '=') return Operator::EqualTo;
            break;
        case '<':
            if (c2 == '=') return Operator::LessThanOrEqualTo;
            break;
        case '>':
            if (c2 == '=') return Operator::GreaterThanOrEqualTo;
            break;
        default:
            break;
    }

    return Operator::Invalid;
}

static inline Operator build_unigraph_operator(char c)
{
    switch (c) {
        case '-':
            return Operator::Minus;
        case '+':
            return Operator::Plus;
        case '/':
            return Operator::Divide;
        case '*':
            return Operator::Multiply;
        case '!':
            return Operator::LogicalNegation;
        case '>':
            return Operator::GreaterThan;
        case '<':
            return Operator::LessThan;
        case '~':
            return Operator::Complement;
        case ':':
            return Operator::Colon;
        case '?':
            return Operator::QuestionMark;
        case '=':
            return Operator::Assignment;
        default:
            return Operator::Invalid;
    }
}

static inline bool is_valid_keyword_identifier_trailing_character(char c)
{
    return is_valid_keyword_identifier_first_character(c) || (c >= '0' && c <= '9');
}

// FIXME: need separate functions for consuming different types of
// literals/identifiers
static void tokenizer_consume_id(Tokenizer* tokenizer)
{
    tokenizer->id[0] = '\0';
    tokenizer->id_len = 0;
    tokenizer_grow_id_buffer(tokenizer);
    tokenizer_advance(tokenizer);

    while (is_valid_keyword_identifier_trailing_character(tokenizer->next_char)) {
        // if (tokenizer.id_len == QXC_MAXIMUM_IDENTIFIER_LENGTH-1) {  // report
        // error
        // }
        tokenizer_grow_id_buffer(tokenizer);
        tokenizer_advance(tokenizer);
    }

    tokenizer->id[tokenizer->id_len] = '\0';
}

static inline bool is_valid_symbol(char c)
{
    return c != '\0' && strchr("{}();", c) != nullptr;
}

static void build_symbol_token(Tokenizer* tokenizer, DynArray<Token>* token_buffer,
                               char c)
{
    Token* new_token = token_buffer->append();
    new_token->line = tokenizer->current_line;
    new_token->column = tokenizer->current_column;

    switch (c) {
        case '{':
            new_token->type = TokenType::OpenBrace;
            break;
        case '}':
            new_token->type = TokenType::CloseBrace;
            break;
        case '(':
            new_token->type = TokenType::OpenParen;
            break;
        case ')':
            new_token->type = TokenType::CloseParen;
            break;
        case ';':
            new_token->type = TokenType::SemiColon;
            break;
        default:
            new_token->type = TokenType::Invalid;
            break;
    }
}

static inline void consume_symbol_token(Tokenizer* tokenizer,
                                        DynArray<Token>* token_buffer)
{
    build_symbol_token(tokenizer, token_buffer, tokenizer->next_char);
    tokenizer_advance(tokenizer);
}

static void build_operator_token(Tokenizer* tokenizer, DynArray<Token>* token_buffer,
                                 Operator op)
{
    Token* new_token = token_buffer->append();
    new_token->type = TokenType::Operator;
    new_token->op = op;
    new_token->line = tokenizer->current_line;
    new_token->column = tokenizer->current_column;
}

// 'consume' == build + advance tokenizer
static void consume_operator_token(Tokenizer* tokenizer, DynArray<Token>* token_buffer,
                                   Operator op)
{
    build_operator_token(tokenizer, token_buffer, op);
    tokenizer_advance(tokenizer);
}

int tokenize(DynArray<Token>* token_buffer, const char* filepath)
{
    token_buffer->clear();

    Tokenizer tokenizer;

    if (tokenizer_init(&tokenizer, filepath) != 0) {
        debug_print("failed to initializer tokenizer");
        return -1;
    }

    while (1) {
        if (is_valid_keyword_identifier_first_character(tokenizer.next_char)) {
            tokenizer_consume_id(&tokenizer);

            Token* new_token = token_buffer->append();

            Keyword keyword = str_to_keyword(tokenizer.id);

            if (keyword == Keyword::Invalid) {
                // it's an identifier, not a keyword
                new_token->type = TokenType::Identifier;

                // TODO: bounds check identifier length
                memcpy(new_token->name, tokenizer.id, (size_t)tokenizer.id_len + 1);
            }
            else {  // it's a built in keyword
                new_token->type = TokenType::KeyWord;
                new_token->keyword = keyword;
            }

            new_token->line = tokenizer.current_line;
            new_token->column = tokenizer.current_column - tokenizer.id_len;
        }

        else if (is_valid_symbol(tokenizer.next_char)) {
            consume_symbol_token(&tokenizer, token_buffer);
        }

        else if (is_valid_operator_first_character(tokenizer.next_char)) {
            char c1 = tokenizer.next_char;
            tokenizer_advance(&tokenizer);
            char c2 = tokenizer.next_char;

            Operator digraph_op = try_build_digraph_operator(c1, c2);

            if (digraph_op != Operator::Invalid) {
                consume_operator_token(&tokenizer, token_buffer, digraph_op);
            }
            else {
                Operator maybe_unigraph_op = build_unigraph_operator(c1);

                if (maybe_unigraph_op != Operator::Invalid) {
                    build_operator_token(&tokenizer, token_buffer, maybe_unigraph_op);
                }
                else if (c1 == '=') {
                    // FIXME: this is messy
                    build_symbol_token(&tokenizer, token_buffer, '=');
                }
                else {
                    fprintf(stderr, "invalid operator-like character encountered: %c\n",
                            c1);
                    return -1;
                }

                // we don't have to advance tokenizer here, since we already consumed the
                // next token when checking for digraph operators above
            }
        }

        else if (tokenizer.next_char == '\n') {
            tokenizer.next_char_ptr++;
            tokenizer.next_char = *tokenizer.next_char_ptr;
            tokenizer.current_line++;
            tokenizer.current_column = 1;
        }
        else if (tokenizer.next_char == ' ') {
            tokenizer_advance(&tokenizer);
        }

        else if (tokenizer.next_char >= '0' && tokenizer.next_char <= '9') {
            tokenizer_consume_id(&tokenizer);

            errno = 0;
            long maybe_value = strtol(tokenizer.id, nullptr, 10);
            if (errno != 0) {
                fprintf(stderr, "failed to parse integer literal\n");
                return -1;
            }

            Token* new_token = token_buffer->append();
            new_token->type = TokenType::IntLiteral;
            new_token->line = tokenizer.current_line;
            new_token->column = tokenizer.current_column - tokenizer.id_len;
            new_token->int_literal_value = maybe_value;
        }

        else if (tokenizer.next_char == '\0') {
            // we're done!
            break;
        }

        else {
            debug_print("encountered unexpected character: %c %d\n", tokenizer.next_char,
                        (int)tokenizer.next_char);
            // TODO: free buffer and return nullptr here
            break;
        }
    }

    tokenizer_free(&tokenizer);

    return 0;
}

