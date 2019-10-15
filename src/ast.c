#include "ast.h"
#include "lexer.h"
#include "prelude.h"
#include "pretty_print_ast.h"
#include "token.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// <program> ::= <function>
// <function> ::= "int" <id> "(" ")" "{" { <block-item> } "}"
// <block-item> ::= <statement> | <declaration>
// <declaration> ::= "int" <id> [ = <exp> ] ";"
// <statement> ::= "return" <exp> ";"
//               | <exp> ";"
//               | "if" "(" <exp> ")" <statement> [ "else" <statement> ]
//
// <exp> ::= <id> "=" <exp> | <conditional-exp>
// <conditional-exp> ::= <logical-or-exp> [ "?" <exp> ":" <conditional-exp> ]
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

static inline struct qxc_ast_block_item_node* alloc_empty_block_item(
    struct qxc_parser* parser)
{
    struct qxc_ast_block_item_node* block_item =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_ast_block_item_node));
    block_item->type = INVALID_BLOCK_ITEM;
    return block_item;
}

static inline struct qxc_ast_statement_node* alloc_empty_statement(
    struct qxc_parser* parser)
{
    struct qxc_ast_statement_node* statement =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_ast_statement_node));
    statement->type = INVALID_STATEMENT;
    return statement;
}

static inline struct qxc_ast_declaration_node* alloc_empty_declaration_node(
    struct qxc_parser* parser)
{
    struct qxc_ast_declaration_node* decl =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_ast_declaration_node));
    decl->var_name = NULL;
    decl->initializer_expr = NULL;
    return decl;
}

static inline struct qxc_block_item_list* alloc_empty_block_item_list(
    struct qxc_parser* parser)
{
    struct qxc_block_item_list* blist =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_block_item_list));
    blist->node = NULL;
    blist->next_node = NULL;
    return blist;
}

static inline struct qxc_ast_function_decl_node* alloc_empty_function_decl(
    struct qxc_parser* parser)
{
    struct qxc_ast_function_decl_node* decl =
        qxc_malloc(parser->ast_memory_pool, sizeof(struct qxc_ast_function_decl_node));

    decl->blist = alloc_empty_block_item_list(parser);
    decl->name = NULL;

    return decl;
}

static void qxc_block_item_list_append(struct qxc_parser* parser,
                                       struct qxc_block_item_list* blist,
                                       struct qxc_ast_block_item_node* new_block_item)
{
    while (blist->node != NULL) {
        blist = blist->next_node;
    }

    blist->node = new_block_item;
    blist->next_node = alloc_empty_block_item_list(parser);
}

// static void rewind_token_buffer(struct qxc_parser* parser, size_t count)
// {
//     while (count > 0 && parser->itoken > 0) {
//         parser->itoken--;
//         count--;
//     }
// }

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

// static struct qxc_token* qxc_parser_expect_operator(struct qxc_parser* parser,
//                                                     enum qxc_operator expected_op)
// {
//     struct qxc_token* next_token = pop_next_token(parser);
//
//     EXPECT(
//         next_token && next_token->type == OPERATOR_TOKEN && next_token->op ==
//         expected_op, "Expected operator");
//
//     return next_token;
// }

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser*);

static struct qxc_ast_expression_node* qxc_parse_factor(struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* factor = alloc_empty_expression(parser);

    struct qxc_token* next_token = pop_next_token(parser);
    EXPECT_(next_token);

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
            factor = qxc_parse_expression(parser);

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

// TODO: clean this up and handle all errors with informative print statements
static int binop_precedence(enum qxc_operator op)
{
    switch (op) {
        case MINUS_OP:
            return 12;
        case PLUS_OP:
            return 12;
        case DIVIDE_OP:
            return 13;
        case MULTIPLY_OP:
            return 13;
        case LOGICAL_AND_OP:
            return 5;
        case LOGICAL_OR_OP:
            return 4;
        case EQUAL_TO_OP:
            return 9;
        case NOT_EQUAL_TO_OP:
            return 9;
        case COLON_OP:
            return 3;
        case QUESTION_MARK_OP:
            return 3;
        case LESS_THAN_OP:
            return 10;
        case LESS_THAN_OR_EQUAL_TO_OP:
            return 10;
        case GREATER_THAN_OP:
            return 10;
        case GREATER_THAN_OR_EQUAL_TO_OP:
            return 10;
        case ASSIGNMENT_OP:
            return 2;
        case COMMA_OP:
            return 1;
        case BITWISE_OR_OP:
            return 6;
        case BITWISE_AND_OP:
            return 8;
        case BITWISE_XOR_OP:
            return 7;
        case BITSHIFT_LEFT_OP:
            return 11;
        case BITSHIFT_RIGHT_OP:
            return 11;
        case PERCENT_OP:
            return 13;
        case INVALID_OP:
            fprintf(stderr, "invalid operator encountered");
            return -666;
        default:
            return -999;
    }
}

static bool binop_valid_for_logical_or_expr_or_below(enum qxc_operator op)
{
    return binop_precedence(op) > 3;
}

// remember, if things get wonky you can re-write the parser explicitely for each
// definition
static struct qxc_ast_expression_node* qxc_parse_logical_or_expr_(
    struct qxc_parser* parser, struct qxc_ast_expression_node* left_factor,
    int min_precedence)
{
    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    while (next_token->type == OPERATOR_TOKEN &&
           binop_valid_for_logical_or_expr_or_below(next_token->op)) {
        EXPECT(next_token->op != ASSIGNMENT_OP, "assignment operator in invalid place!");
        const int next_op_precedence = binop_precedence(next_token->op);

        if (next_op_precedence <= min_precedence) {
            break;
        }

        next_token = pop_next_token(parser);
        struct qxc_ast_expression_node* right_expr = qxc_parse_logical_or_expr_(
            parser, qxc_parse_factor(parser), next_op_precedence);
        EXPECT_(right_expr);

        struct qxc_ast_expression_node* binop_expr = alloc_empty_expression(parser);
        binop_expr->type = BINARY_OP_EXPR;
        binop_expr->binop = next_token->op;
        binop_expr->left_expr = left_factor;
        binop_expr->right_expr = right_expr;

        left_factor = binop_expr;

        next_token = peek_next_token(parser);
        EXPECT_(next_token);
    }

    return left_factor;
}

static struct qxc_ast_expression_node* qxc_parse_logical_or_expr(
    struct qxc_parser* parser, struct qxc_ast_expression_node* left_factor)
{
    return qxc_parse_logical_or_expr_(parser, left_factor, -1);
}

static struct qxc_ast_expression_node* qxc_parse_conditional_expression(
    struct qxc_parser* parser, struct qxc_ast_expression_node* left_factor)
{
    struct qxc_ast_expression_node* lor_expr =
        qxc_parse_logical_or_expr(parser, left_factor);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    if (next_token->op == QUESTION_MARK_OP) {
        (void)pop_next_token(parser);

        struct qxc_ast_expression_node* ternary_expr = alloc_empty_expression(parser);
        ternary_expr->type = CONDITIONAL_EXPR;
        ternary_expr->conditional_expr = lor_expr;
        ternary_expr->if_expr = qxc_parse_expression(parser);
        (void)pop_next_token(parser);
        ternary_expr->else_expr =
            qxc_parse_conditional_expression(parser, qxc_parse_factor(parser));

        return ternary_expr;
    }
    else {
        return lor_expr;
    }
}

static struct qxc_ast_expression_node* qxc_parse_expression(struct qxc_parser* parser)
{
    struct qxc_ast_expression_node* left_factor = qxc_parse_factor(parser);
    EXPECT_(left_factor);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    // assignment operator is right-associative, so special treatment here
    if (next_token->op == ASSIGNMENT_OP) {
        EXPECT(left_factor->type == VARIABLE_REFERENCE_EXPR,
               "left hand side of assignment operator must be a variable reference!");
        (void)pop_next_token(parser);

        struct qxc_ast_expression_node* binop_expr = alloc_empty_expression(parser);
        binop_expr->type = BINARY_OP_EXPR;
        binop_expr->binop = ASSIGNMENT_OP;
        binop_expr->left_expr = left_factor;
        binop_expr->right_expr = qxc_parse_expression(parser);

        return binop_expr;
    }
    else {
        return qxc_parse_conditional_expression(parser, left_factor);
    }
}

static struct qxc_ast_declaration_node* qxc_parse_declaration(struct qxc_parser* parser)
{
    struct qxc_token* next_token = pop_next_token(parser);  // pop off int keyword
    assert(next_token->type == KEYWORD_TOKEN && next_token->keyword == INT_KEYWORD);

    struct qxc_ast_declaration_node* new_declaration =
        alloc_empty_declaration_node(parser);

    next_token = pop_next_token(parser);  // pop off identifier
    EXPECT(next_token && next_token->type == IDENTIFIER_TOKEN,
           "Invalid identifier found for variable declaration name");

    debug_print("parsing declaration of int var: %s", next_token->name);

    const size_t id_len = strlen(next_token->name) + 1;  // includes \0 terminator
    new_declaration->var_name = qxc_malloc(parser->ast_memory_pool, id_len);
    memcpy(new_declaration->var_name, next_token->name, id_len);

    next_token = peek_next_token(parser);

    if (next_token && next_token->type == OPERATOR_TOKEN &&
        next_token->op == ASSIGNMENT_OP) {
        pop_next_token(parser);  // pop off '='
        new_declaration->initializer_expr = qxc_parse_expression(parser);
        EXPECT(new_declaration->initializer_expr, "Failed to parse variable initializer");
    }

    EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
           "Missing semicolon at end of declaration");

    return new_declaration;
}

static struct qxc_ast_statement_node* qxc_parse_statement(struct qxc_parser* parser)
{
    struct qxc_token* next_token = peek_next_token(parser);

    struct qxc_ast_statement_node* statement = alloc_empty_statement(parser);

    if (next_token->type == KEYWORD_TOKEN && next_token->keyword == RETURN_KEYWORD) {
        pop_next_token(parser);  // pop off return keyword
        statement->type = RETURN_STATEMENT;
        statement->return_expr = qxc_parse_expression(parser);
        EXPECT(statement->return_expr, "Expression parsing failed");

        EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
               "Missing semicolon at end of statement");
    }
    else if (next_token->type == KEYWORD_TOKEN && next_token->keyword == IF_KEYWORD) {
        (void)pop_next_token(parser);  // pop off 'if' keyword
        statement->type = CONDITIONAL_STATEMENT;
        statement->conditional_expr = qxc_parse_expression(parser);
        EXPECT_(statement->conditional_expr);
        statement->if_branch_statement = qxc_parse_statement(parser);
        EXPECT_(statement->if_branch_statement);

        next_token = peek_next_token(parser);

        if (next_token && next_token->type == KEYWORD_TOKEN &&
            next_token->keyword == ELSE_KEYWORD) {
            (void)pop_next_token(parser);  // pop off 'else' keyword
            statement->else_branch_statement = qxc_parse_statement(parser);
            EXPECT_(statement->else_branch_statement);
        }
    }
    else {
        // fallthrough, so attempt to parse standalone expression statement
        // for now the only meaningful stand-alone expression is variable assignment
        // i.e. a = 2;
        debug_print("attempting to parse standalone expression");
        struct qxc_ast_expression_node* standalone_expression =
            qxc_parse_expression(parser);

        EXPECT(standalone_expression, "Failed to parse standalone expression");

        statement->type = EXPRESSION_STATEMENT;
        statement->standalone_expr = standalone_expression;

        EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
               "Missing semicolon at end of statement");
    }

    return statement;
}

static struct qxc_ast_block_item_node* qxc_parse_block_item(struct qxc_parser* parser)
{
    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT(next_token, "Expected another block item here!");

    struct qxc_ast_block_item_node* block_item = alloc_empty_block_item(parser);

    if (next_token->type == KEYWORD_TOKEN && next_token->keyword == INT_KEYWORD) {
        block_item->type = DECLARATION_BLOCK_ITEM;
        block_item->declaration = qxc_parse_declaration(parser);
        EXPECT_(block_item->declaration);
    }
    else {
        block_item->type = STATEMENT_BLOCK_ITEM;
        block_item->statement = qxc_parse_statement(parser);
        EXPECT_(block_item->statement);
    }

    return block_item;
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

    while (peek_next_token(parser)->type != CLOSE_BRACE_TOKEN) {
        struct qxc_ast_block_item_node* next_block_item = qxc_parse_block_item(parser);
        EXPECT(next_block_item, "Failed to parse block item in function: main");
        qxc_block_item_list_append(parser, decl->blist, next_block_item);
        debug_print("successfully parsed block item");
    }

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
