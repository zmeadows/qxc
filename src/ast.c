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

// TODO: remove qxc_ prefix from static functions

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

static void qxc_statement_list_append(struct qxc_statement_list* slist,
                                      struct qxc_ast_statement_node* new_statement_node)
{
    while (slist->node) {
        slist = slist->next_node;
    }

    slist->node = new_statement_node;
    slist->next_node = qxc_malloc(sizeof(struct qxc_statement_list));
}

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

    EXPECT(next_token && next_token->type == KEYWORD_TOKEN, "Expected keyword token");
    EXPECT(next_token->keyword == expected_keyword, "Unexpected keyword token: %s",
           qxc_keyword_to_str(next_token->keyword));

    return next_token;
}

static struct qxc_token* qxc_parser_expect_identifier(struct qxc_parser* parser,
                                                      const char* expected_identifier)
{
    struct qxc_token* next_token = pop_next_token(parser);

    EXPECT(next_token && next_token->type == IDENTIFIER_TOKEN &&
               strs_are_equal(next_token->name, expected_identifier),
           "Expected identifier");

    return next_token;
}

static inline struct qxc_ast_expression_node* new_expr_node(void)
{
    struct qxc_ast_expression_node* expr =
        qxc_malloc(sizeof(struct qxc_ast_expression_node));
    expr->type = INVALID_EXPR;
    return expr;
}

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser*, int);

static struct qxc_ast_expression_node* qxc_parse_factor(struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* factor = new_expr_node();

    struct qxc_token* next_token = pop_next_token(parser);
    switch (next_token->type) {
        case INTEGER_LITERAL_TOKEN:
            factor->type = INT_LITERAL_EXPR;
            factor->literal = next_token->int_literal_value;

            break;

        case OPERATOR_TOKEN:
            EXPECT(qxc_operator_can_be_unary(next_token->op),
                   "non-unary operator in unary operator context: %s",
                   qxc_operator_to_str(next_token->op));
            factor->type = UNARY_OP_EXPR;
            factor->unop = next_token->op;
            factor->unary_expr = qxc_parse_factor(parser);
            EXPECT(factor->unary_expr,
                   "Failed to parse child expression of unary operator");
            break;

        case OPEN_PAREN_TOKEN:
            factor = qxc_parse_expression(parser, -1);

            EXPECT_(factor);

            EXPECT(qxc_parser_expect_token_type(parser, CLOSE_PAREN_TOKEN),
                   "Missing close parenthesis after enclosed factor");

            break;

        default:
            return NULL;
    }

    return factor;
}

static int binop_precedence(enum qxc_operator op)
{
    switch (op) {
        case LOGICAL_AND_OP:
            return 2;
        case LOGICAL_OR_OP:
            return 1;
        case MULTIPLY_OP:
            return 6;
        case DIVIDE_OP:
            return 6;
        case PLUS_OP:
            return 5;
        case MINUS_OP:
            return 5;
        case EQUAL_TO_OP:
            return 3;
        case NOT_EQUAL_TO_OP:
            return 3;
        case LESS_THAN_OP:
            return 4;
        case LESS_THAN_OR_EQUAL_TO_OP:
            return 4;
        case GREATER_THAN_OP:
            return 4;
        case GREATER_THAN_OR_EQUAL_TO_OP:
            return 4;
        default:
            return -1;
    }
}

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser* parser,
                                                            int min_precedence)
{
    struct qxc_ast_expression_node* left_expr = qxc_parse_factor(parser);
    EXPECT_(left_expr);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    while (next_token->type == OPERATOR_TOKEN) {
        int next_op_precedence = binop_precedence(next_token->op);

        if (next_op_precedence <= min_precedence) {
            break;
        }

        next_token = pop_next_token(parser);
        struct qxc_ast_expression_node* right_expr =
            qxc_parse_expression(parser, next_op_precedence);  // descend
        EXPECT_(right_expr);

        struct qxc_ast_expression_node* binop_expr = new_expr_node();
        binop_expr->type = BINARY_OP_EXPR;
        binop_expr->binop = next_token->op;
        binop_expr->left_expr = left_expr;
        binop_expr->right_expr = right_expr;

        left_expr = binop_expr;

        next_token = peek_next_token(parser);
        EXPECT_(next_token);
    }

    return left_expr;
}

static struct qxc_ast_statement_node* qxc_parse_statement(struct qxc_parser* parser)
{
    EXPECT(qxc_parser_expect_keyword(parser, RETURN_KEYWORD),
           "Only return statements currently allowed.");

    struct qxc_ast_statement_node* statement =
        qxc_malloc(sizeof(struct qxc_ast_statement_node));

    statement->type = RETURN_STATEMENT;
    statement->return_expr = qxc_parse_expression(parser, 0);

    EXPECT(statement->return_expr, "Expression parsing failed");

    EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
           "Missing semicolon at end of statement");

    return statement;
}

static struct qxc_ast_function_decl_node* qxc_parse_function_decl(
    struct qxc_parser* parser, const char* func_name)
{
    EXPECT(qxc_parser_expect_token_type(parser, OPEN_PAREN_TOKEN),
           "Missing open parenthesis");
    EXPECT(qxc_parser_expect_token_type(parser, CLOSE_PAREN_TOKEN),
           "Missing close parenthesis");
    EXPECT(qxc_parser_expect_token_type(parser, OPEN_BRACE_TOKEN),
           "Missing open brace token");

    struct qxc_ast_function_decl_node* decl =
        qxc_malloc(sizeof(struct qxc_ast_function_decl_node));
    decl->slist = qxc_malloc(sizeof(struct qxc_statement_list));
    decl->slist->node = NULL;
    decl->slist->next_node = NULL;
    decl->name = func_name;

    // node->statement = qxc_parse_statement(parser);
    while (peek_next_token(parser)->type != CLOSE_BRACE_TOKEN) {
        qxc_statement_list_append(decl->slist, qxc_parse_statement(parser));
    }

    // TODO : check for errors on each statement parse
    EXPECT(decl->slist->node, "failed to parse function body");

    EXPECT(qxc_parser_expect_token_type(parser, CLOSE_BRACE_TOKEN),
           "Missing close brace token");

    return decl;
}

struct qxc_program* qxc_parse(const char* filepath)
{
    struct qxc_parser* parser = qxc_malloc(sizeof(struct qxc_parser));
    parser->token_buffer = qxc_tokenize(filepath);
    if (!parser->token_buffer) {
        return NULL;
    }
    parser->itoken = 0;

    EXPECT(qxc_parser_expect_keyword(parser, INT_KEYWORD),
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
