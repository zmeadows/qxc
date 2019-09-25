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

#define EXPECT_(EXPR)    \
    do {                 \
        if (!(EXPR)) {   \
            return NULL; \
        }                \
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

static struct qxc_token* peek_next_token(struct qxc_parser* parser)
{
    if (parser->itoken < parser->token_buffer->length) {
        return &parser->token_buffer->tokens[parser->itoken];
    }
    else {
        return NULL;
    }
}

static struct qxc_token* qxc_parser_expect_token_type(
    struct qxc_parser* parser, enum qxc_token_type expected_token_type)
{
    struct qxc_token* next_token = pop_next_token(parser);
    EXPECT_(next_token && next_token->type == expected_token_type);
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

static inline bool is_plus_minus_token(struct qxc_token* token)
{
    return token->type == qxc_operator_token &&
           (token->op == qxc_minus_op || token->op == qxc_plus_op);
}

static inline bool is_divide_multiply_token(struct qxc_token* token)
{
    return token->type == qxc_operator_token &&
           (token->op == qxc_divide_op || token->op == qxc_multiply_op);
}

static inline struct qxc_ast_expression_node* new_expr_node(void)
{
    struct qxc_ast_expression_node* expr =
        qxc_malloc(sizeof(struct qxc_ast_expression_node));
    expr->type = INVALID_EXPR;
    return expr;
}

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser* parser);

static struct qxc_ast_expression_node* qxc_parse_factor(struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* factor = new_expr_node();

    struct qxc_token* next_token = pop_next_token(parser);
    switch (next_token->type) {
        case qxc_integer_literal_token:
            factor->type = INT_LITERAL_EXPR;
            factor->literal = next_token->int_literal_value;

            break;

        case qxc_operator_token:
            EXPECT(qxc_operator_can_be_unary(next_token->op),
                   "non-unary operator in unary operator context: %c",
                   qxc_operator_to_char(next_token->op));
            factor->type = UNARY_OP_EXPR;
            factor->unop = next_token->op;
            factor->unary_expr = qxc_parse_factor(parser);
            EXPECT(factor->unary_expr,
                   "Failed to parse child expression of unary operator");
            break;

        case qxc_open_paren_token:
            factor = qxc_parse_expression(parser);

            EXPECT_(factor);

            EXPECT(qxc_parser_expect_token_type(parser, qxc_close_paren_token),
                   "Missing close parenthesis after enclosed factor");

            break;

        default:
            return NULL;
    }

    return factor;
}

static struct qxc_ast_expression_node* qxc_parse_term(struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* term = qxc_parse_factor(parser);
    EXPECT_(term);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    while (is_divide_multiply_token(next_token)) {
        next_token = pop_next_token(parser);

        struct qxc_ast_expression_node* rhs = qxc_parse_factor(parser);
        EXPECT_(rhs);

        struct qxc_ast_expression_node* upper_term = new_expr_node();
        upper_term->type = BINARY_OP_EXPR;
        upper_term->binop = next_token->op;
        upper_term->left_expr = term;
        upper_term->right_expr = rhs;

        term = upper_term;

        next_token = peek_next_token(parser);
        EXPECT_(next_token);
    }

    return term;
}

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* expr = qxc_parse_term(parser);
    EXPECT_(expr);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    while (is_plus_minus_token(next_token)) {
        next_token = pop_next_token(parser);

        struct qxc_ast_expression_node* rhs = qxc_parse_term(parser);
        EXPECT_(rhs);

        struct qxc_ast_expression_node* upper_expr =
            qxc_malloc(sizeof(struct qxc_ast_expression_node));
        upper_expr->type = BINARY_OP_EXPR;
        upper_expr->binop = next_token->op;
        upper_expr->left_expr = expr;
        upper_expr->right_expr = rhs;

        expr = upper_expr;

        next_token = peek_next_token(parser);
        EXPECT_(next_token);
    }

    return expr;
}

static struct qxc_ast_statement_node* qxc_parse_statement(struct qxc_parser* parser)
{
    EXPECT(qxc_parser_expect_keyword(parser, qxc_return_keyword),
           "Only return statements currently allowed.");

    struct qxc_ast_statement_node* statement =
        qxc_malloc(sizeof(struct qxc_ast_statement_node));

    statement->type = RETURN_STATEMENT;
    statement->expr = qxc_parse_expression(parser);

    EXPECT(statement->expr, "Expression parsing failed");

    EXPECT(qxc_parser_expect_token_type(parser, qxc_semicolon_token),
           "Missing semicolon at end of statement");

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

    qxc_token_buffer_free(parser->token_buffer);
    return program;
}
