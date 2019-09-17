#include "ast.h"
#include "allocator.h"
#include "lexer.h"
#include "prelude.h"
#include "token.h"

#include <assert.h>
#include <stdio.h>

struct qxc_parser {
    struct qxc_token_buffer* token_buffer;
    size_t itoken;
};

#define EXPECT(EXPR, ERRMSG)         \
    do {                             \
        if (!(EXPR)) {               \
            fprintf(stderr, ERRMSG); \
            return NULL;             \
        }                            \
    } while (0)

static struct qxc_token* qxc_parser_next_token(struct qxc_parser* parser) {
    return &parser->token_buffer->tokens[parser->itoken];
}

static struct qxc_token* qxc_parser_expect_token_type(
    struct qxc_parser* parser, enum qxc_token_type expected_token_type) {
    struct qxc_token* next_token = qxc_parser_next_token(parser);

    if (next_token->type == expected_token_type) {
        parser->itoken++;
        return next_token;
    } else {
        printf("%d\n", __LINE__);
        return NULL;
    }
}

static struct qxc_token* qxc_parser_expect_keyword(
    struct qxc_parser* parser, enum qxc_keyword expected_keyword) {
    struct qxc_token* next_token = qxc_parser_next_token(parser);

    if (next_token->type == qxc_keyword_token &&
        next_token->keyword == expected_keyword) {
        parser->itoken++;
        return next_token;
    } else {
        qxc_token_print(next_token);
        printf("%d\n", __LINE__);
        return NULL;
    }
}

static struct qxc_token* qxc_parser_expect_identifier(
    struct qxc_parser* parser, const char* expected_identifier) {
    struct qxc_token* next_token = qxc_parser_next_token(parser);

    if (next_token->type == qxc_identifier_token &&
        strs_are_equal(next_token->name, expected_identifier)) {
        parser->itoken++;
        return next_token;
    } else {
        printf("%d\n", __LINE__);
        return NULL;
    }
}

static struct qxc_ast_expression_node* qxc_parse_expression(
    struct qxc_parser* parser) {
    struct qxc_token* return_value_token =
        qxc_parser_expect_token_type(parser, qxc_integer_literal_token);

    if (!return_value_token) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    struct qxc_ast_expression_node* expr =
        qxc_malloc(sizeof(struct qxc_ast_expression_node));

    expr->type = qxc_int_literal_expr;
    expr->value = return_value_token->value;

    if (!qxc_parser_expect_token_type(parser, qxc_semicolon_token)) {
        fprintf(stderr, "Expected semicolon after return value");
        return NULL;
    }

    if (!qxc_parser_expect_token_type(parser, qxc_close_brace_token)) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    return expr;
}

static struct qxc_ast_statement_node* qxc_parse_statement(
    struct qxc_parser* parser) {
    if (!qxc_parser_expect_keyword(parser, qxc_return_keyword)) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    struct qxc_ast_statement_node* statement =
        qxc_malloc(sizeof(struct qxc_ast_statement_node));

    statement->type = qxc_return_statement;
    statement->expr = qxc_parse_expression(parser);

    if (!statement->expr) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    return statement;
}

static struct qxc_ast_function_decl_node* qxc_parse_function_decl(
    struct qxc_parser* parser, const char* func_name) {
    if (!qxc_parser_expect_token_type(parser, qxc_open_paren_token)) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    if (!qxc_parser_expect_token_type(parser, qxc_close_paren_token)) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    if (!qxc_parser_expect_token_type(parser, qxc_open_brace_token)) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    struct qxc_ast_function_decl_node* node =
        qxc_malloc(sizeof(struct qxc_ast_function_decl_node));

    node->name = func_name;

    node->statement = qxc_parse_statement(parser);

    if (!node->statement) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    return node;
}

struct qxc_program* qxc_parse(const char* filepath) {
    struct qxc_parser* parser = qxc_malloc(sizeof(struct qxc_parser));

    parser->token_buffer = qxc_tokenize(filepath);
    assert(parser->token_buffer);
    parser->itoken = 0;

    if (!qxc_parser_expect_keyword(parser, qxc_int_keyword)) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    if (!qxc_parser_expect_identifier(parser, "main")) {
        printf("%d\n", __LINE__);
        return NULL;
    }

    struct qxc_ast_function_decl_node* main_decl =
        qxc_parse_function_decl(parser, "main");

    if (!main_decl) {
        return NULL;
    }

    struct qxc_program* program = qxc_malloc(sizeof(struct qxc_program));

    return program;
}
