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

#define EXPECT(EXPR, ...)                                                  \
    do {                                                                   \
        if (!(EXPR)) {                                                     \
            fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __func__); \
            fprintf(stderr, __VA_ARGS__);                                  \
            fprintf(stderr, "\n");                                         \
            return NULL;                                                   \
        }                                                                  \
    } while (0)

static struct qxc_token* pop_next_token(struct qxc_parser* parser)
{
    if (parser->itoken < parser->token_buffer->length) {
        return &parser->token_buffer->tokens[parser->itoken++];
    }
    else {
        return NULL;
    }
}

// static struct qxc_token* peek_next_token(struct qxc_parser* parser)
// {
//     if (parser->itoken < parser->token_buffer->length) {
//         return &parser->token_buffer->tokens[parser->itoken];
//     }
//     else {
//         return NULL;
//     }
// }

static struct qxc_token* qxc_parser_expect_token_type(
    struct qxc_parser* parser, enum qxc_token_type expected_token_type)
{
    struct qxc_token* next_token = pop_next_token(parser);
    EXPECT(next_token && next_token->type == expected_token_type,
           "Unexpected token type encountered");
    return next_token;
}

static struct qxc_token* qxc_parser_expect_keyword(struct qxc_parser* parser,
                                                   enum qxc_keyword expected_keyword)
{
    struct qxc_token* next_token = pop_next_token(parser);

    EXPECT(next_token && next_token->type == qxc_keyword_token, "Expected keyword token");
    EXPECT(next_token->keyword == expected_keyword, "Unexpected keyword token: %s",
           qxc_keyword_to_str(next_token->keyword));

    return next_token;
}

static struct qxc_token* qxc_parser_expect_identifier(struct qxc_parser* parser,
                                                      const char* expected_identifier)
{
    struct qxc_token* next_token = pop_next_token(parser);

    EXPECT(next_token && next_token->type == qxc_identifier_token &&
               strs_are_equal(next_token->name, expected_identifier),
           "Expected identifier");

    return next_token;
}

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser* parser)
{
    struct qxc_token* next_token = pop_next_token(parser);
    EXPECT(next_token, "File ended too soon");

    struct qxc_ast_expression_node* expr =
        qxc_malloc(sizeof(struct qxc_ast_expression_node));

    switch (next_token->type) {
        case qxc_integer_literal_token:
            expr->type = qxc_int_literal_expr;
            expr->int_literal_value = next_token->int_literal_value;

            EXPECT(qxc_parser_expect_token_type(parser, qxc_semicolon_token),
                   "Missing semicolon");
            break;

        case qxc_operator_token:
            expr->type = qxc_unary_op_expr;
            expr->unary_op = next_token->op;
            expr->child_expr = qxc_parse_expression(parser);
            EXPECT(expr->child_expr,
                   "Failed to parse child expression of unary operator");
            break;

        default:
            return NULL;
    }

    return expr;
}

static struct qxc_ast_statement_node* qxc_parse_statement(struct qxc_parser* parser)
{
    EXPECT(qxc_parser_expect_keyword(parser, qxc_return_keyword),
           "Only return statements currently allowed.");

    struct qxc_ast_statement_node* statement =
        qxc_malloc(sizeof(struct qxc_ast_statement_node));

    statement->type = qxc_return_statement;
    statement->expr = qxc_parse_expression(parser);

    EXPECT(statement->expr, "Expression parsing failed");

    return statement;
}

static struct qxc_ast_function_decl_node* qxc_parse_function_decl(
    struct qxc_parser* parser, const char* func_name)
{
    EXPECT(qxc_parser_expect_token_type(parser, qxc_open_paren_token),
           "Missing open parenthesis");
    EXPECT(qxc_parser_expect_token_type(parser, qxc_close_paren_token),
           "Missing close parenthesis");
    EXPECT(qxc_parser_expect_token_type(parser, qxc_open_brace_token),
           "Missing open brace token");

    struct qxc_ast_function_decl_node* node =
        qxc_malloc(sizeof(struct qxc_ast_function_decl_node));

    node->name = func_name;

    node->statement = qxc_parse_statement(parser);

    EXPECT(node->statement, "failed to parse function body");

    EXPECT(qxc_parser_expect_token_type(parser, qxc_close_brace_token),
           "Missing close brace token");

    return node;
}

// TODO
// void qxc_program_free(struct qxc_program* program) {}

struct qxc_program* qxc_parse(const char* filepath)
{
    struct qxc_parser* parser = qxc_malloc(sizeof(struct qxc_parser));
    parser->token_buffer = qxc_tokenize(filepath);
    if (!parser->token_buffer) {
        return NULL;
    }
    parser->itoken = 0;

    EXPECT(qxc_parser_expect_keyword(parser, qxc_int_keyword),
           "Invalid main type signature, must return int.");

    EXPECT(qxc_parser_expect_identifier(parser, "main"), "Invalid main function name");

    struct qxc_ast_function_decl_node* main_decl =
        qxc_parse_function_decl(parser, "main");

    EXPECT(main_decl, "Failed to parse main function declaration");

    struct qxc_program* program = qxc_malloc(sizeof(struct qxc_program));
    program->main_decl = main_decl;

    return program;
}
