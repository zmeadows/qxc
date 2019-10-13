#include "ast.h"
#include "lexer.h"
#include "prelude.h"
#include "token.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// <program> ::= <function>
// <function> ::= "int" <id> "(" ")" "{" { <statement> } "}"
// <statement> ::= "return" <exp> ";"
//               | <exp> ";"
//               | "int" <id> [ = <exp>] ";"
// <exp> ::= <id> "=" <exp> | <logical-or-exp>
// <logical-or-exp> ::= <logical-and-exp> { "||" <logical-and-exp> }
// <logical-and-exp> ::= <equality-exp> { "&&" <equality-exp> }
// <equality-exp> ::= <relational-exp> { ("!=" | "==") <relational-exp> }
// <relational-exp> ::= <additive-exp> { ("<" | ">" | "<=" | ">=") <additive-exp> }
// <additive-exp> ::= <term> { ("+" | "-") <term> }
// <term> ::= <factor> { ("*" | "/") <factor> }
// <factor> ::= "(" <exp> ")" | <unary_op> <factor> | <int> | <id>
// <unary_op> ::= "!" | "~" | "-"

struct qxc_parser {
    struct qxc_memory_pool* ast_memory_pool;
    struct qxc_token_array token_buffer;
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

static inline struct qxc_ast_expression_node* alloc_empty_expression(
    struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* expr =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_ast_expression_node));
    expr->type = INVALID_EXPR;
    return expr;
}

static inline struct qxc_ast_statement_node* alloc_empty_statement(
    struct qxc_parser* parser)
{
    struct qxc_ast_statement_node* statement =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_ast_statement_node));
    statement->type = INVALID_STATEMENT;
    return statement;
}

static inline struct qxc_statement_list* alloc_empty_statement_list(
    struct qxc_parser* parser)
{
    struct qxc_statement_list* slist =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_statement_list));
    slist->node = NULL;
    slist->next_node = NULL;
    return slist;
}

static inline struct qxc_ast_function_decl_node* alloc_empty_function_decl(
    struct qxc_parser* parser)
{
    struct qxc_ast_function_decl_node* decl =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_ast_function_decl_node));

    decl->slist = alloc_empty_statement_list(parser);
    decl->name = NULL;

    return decl;
}

static void qxc_statement_list_append(struct qxc_parser* parser,
                                      struct qxc_statement_list* slist,
                                      struct qxc_ast_statement_node* statement_to_append)
{
    while (slist->node != NULL) {
        slist = slist->next_node;
    }

    slist->node = statement_to_append;
    slist->next_node = alloc_empty_statement_list(parser);
}

static void rewind_token_buffer(struct qxc_parser* parser, size_t count)
{
    while (count > 0 && parser->itoken > 0) {
        parser->itoken--;
        count--;
    }
}

static struct qxc_token* pop_next_token(struct qxc_parser* parser)
{
    if (parser->itoken < parser->token_buffer.length) {
        return qxc_token_array_at(&parser->token_buffer, parser->itoken++);
    }
    else {
        return NULL;
    }
}

static struct qxc_token* peek_next_token(struct qxc_parser* parser)
{
    if (parser->itoken < parser->token_buffer.length) {
        return qxc_token_array_at(&parser->token_buffer, parser->itoken);
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

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser*, int);

static struct qxc_ast_expression_node* qxc_parse_factor(struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* factor = alloc_empty_expression(parser);

    struct qxc_token* next_token = pop_next_token(parser);
    switch (next_token->type) {
        case INTEGER_LITERAL_TOKEN:
            debug_print("parsed integer literal factor: %ld",
                        next_token->int_literal_value);
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
            debug_print("attempting to parse paren closed expression...");
            factor = qxc_parse_expression(parser, -1);

            EXPECT(factor, "failed to parse paren-closed expression");

            EXPECT(qxc_parser_expect_token_type(parser, CLOSE_PAREN_TOKEN),
                   "Missing close parenthesis after enclosed factor");

            debug_print("found close paren for enclosed expression");

            break;

        case IDENTIFIER_TOKEN:
            factor->type = VARIABLE_REFERENCE_EXPR;
            factor->referenced_var_name = next_token->name;
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
    struct qxc_token* next_token = peek_next_token(parser);

    if (next_token->type == IDENTIFIER_TOKEN) {
        // encountered either a variable reference or variable assignment
        struct qxc_token* id_token = pop_next_token(parser);
        next_token = peek_next_token(parser);

        if (next_token->type == ASSIGNMENT_TOKEN) {
            (void)pop_next_token(parser);
            debug_print("parsing assignment of var: %s", id_token->name);

            struct qxc_ast_expression_node* assignment_expr =
                alloc_empty_expression(parser);
            assignment_expr->type = ASSIGNMENT_EXPR;
            assignment_expr->assignee_var_name = id_token->name;
            assignment_expr->assignment_expr = qxc_parse_expression(parser, -1);

            EXPECT(assignment_expr->assignment_expr,
                   "failed to parse assignment expression for variable: %s",
                   id_token->name);
            debug_print("successfully parsed assignment expression of variable: %s",
                        id_token->name);
            return assignment_expr;
        }
        else {
            rewind_token_buffer(parser, 1);
        }
    }

    struct qxc_ast_expression_node* left_expr = qxc_parse_factor(parser);
    EXPECT_(left_expr);

    next_token = peek_next_token(parser);
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

        struct qxc_ast_expression_node* binop_expr = alloc_empty_expression(parser);
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
    struct qxc_ast_statement_node* statement = alloc_empty_statement(parser);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT(next_token, "Expected a statement here!");

    if (next_token->type == KEYWORD_TOKEN && next_token->keyword == RETURN_KEYWORD) {
        pop_next_token(parser);  // pop off return keyword
        statement->type = RETURN_STATEMENT;
        statement->return_expr = qxc_parse_expression(parser, 0);
        EXPECT(statement->return_expr, "Expression parsing failed");
    }
    else if (next_token->type == KEYWORD_TOKEN && next_token->keyword == INT_KEYWORD) {
        pop_next_token(parser);  // pop off int keyword
        statement->type = DECLARATION_STATEMENT;
        next_token = pop_next_token(parser);  // pop off identifier
        EXPECT(next_token && next_token->type == IDENTIFIER_TOKEN,
               "Invalid identifier found for variable declaration name");

        debug_print("parsing declaration of int var: %s", next_token->name);

        const size_t id_len = strlen(next_token->name) + 1;  // includes \0 terminator
        statement->var_name = qxc_malloc(parser->ast_memory_pool, id_len);
        memcpy(statement->var_name, next_token->name, id_len);

        next_token = peek_next_token(parser);
        if (next_token && next_token->type == ASSIGNMENT_TOKEN) {
            pop_next_token(parser);  // pop off '='
            statement->initializer_expr = qxc_parse_expression(parser, 0);
            EXPECT(statement->initializer_expr, "Failed to parse variable initializer");
        }
    }
    else {
        // fallthrough, so attempt to parse standalone expression statement
        // for now the only meaningful stand-alone expression is variable assignment
        // i.e. a = 2;
        debug_print("attemting to parse standalone expression");
        struct qxc_ast_expression_node* standalone_expression =
            qxc_parse_expression(parser, -1);

        EXPECT(standalone_expression, "Failed to parse standalone expression");

        statement->type = EXPRESSION_STATEMENT;
        statement->standalone_expr = standalone_expression;
    }

    EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
           "Missing semicolon at end of statement");

    return statement;
}

static struct qxc_ast_function_decl_node* qxc_parse_function_decl(
    struct qxc_parser* parser)
{
    EXPECT(qxc_parser_expect_keyword(parser, INT_KEYWORD),
           "Invalid main type signature, must return int.");

    EXPECT(qxc_parser_expect_identifier(parser, "main"), "Invalid main function name");

    EXPECT(qxc_parser_expect_token_type(parser, OPEN_PAREN_TOKEN),
           "Missing open parenthesis");

    // arg parsing would go here, if we allowed function arguments yet

    EXPECT(qxc_parser_expect_token_type(parser, CLOSE_PAREN_TOKEN),
           "Missing close parenthesis");

    EXPECT(qxc_parser_expect_token_type(parser, OPEN_BRACE_TOKEN),
           "Missing open brace token");

    struct qxc_ast_function_decl_node* decl = alloc_empty_function_decl(parser);
    decl->name = "main";

    // bool found_return_statement = false;
    while (peek_next_token(parser)->type != CLOSE_BRACE_TOKEN) {
        struct qxc_ast_statement_node* next_statement = qxc_parse_statement(parser);
        EXPECT(next_statement, "Failed to parse statement in function: main");
        qxc_statement_list_append(parser, decl->slist, next_statement);
        debug_print("successfully parsed statement");
        // if (next_statement->type == RETURN_STATEMENT) {
        //     found_return_statement = true;
        // }
    }

    // EXPECT(found_return_statement,
    //        "No return statement in function with non-void return type!");

    // TODO : check for errors on each statement parse
    // EXPECT(decl->slist->node, "failed to parse any statements in function body of
    // main");

    EXPECT(qxc_parser_expect_token_type(parser, CLOSE_BRACE_TOKEN),
           "Missing close brace token at end of function: main");

    return decl;
}

struct qxc_program* qxc_parse(const char* filepath)
{
    struct qxc_parser parser;

    parser.ast_memory_pool = qxc_memory_pool_init(10e3);
    if (parser.ast_memory_pool == NULL) {
        return NULL;
    }

    if (qxc_token_array_init(&parser.token_buffer, 256) != 0) {
        return NULL;
    }

    if (qxc_tokenize(&parser.token_buffer, filepath) != 0) {
        qxc_token_array_free(&parser.token_buffer);  // finished with tokens now
        return NULL;
    }

    parser.itoken = 0;

    struct qxc_ast_function_decl_node* main_decl = qxc_parse_function_decl(&parser);

    EXPECT(main_decl, "Failed to parse main function declaration");

    qxc_token_array_free(&parser.token_buffer);  // finished with tokens now

    struct qxc_program* program =
        qxc_malloc(parser.ast_memory_pool, sizeof(struct qxc_program));
    program->main_decl = main_decl;
    program->ast_memory_pool = parser.ast_memory_pool;

    return program;
}
