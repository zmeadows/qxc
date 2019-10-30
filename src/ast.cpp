#include "ast.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "lexer.h"
#include "prelude.h"
#include "pretty_print_ast.h"
#include "token.h"

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

struct Parser {
    struct qxc_memory_pool* pool;
    array<struct qxc_token> token_buffer;
    size_t itoken;

    static Parser create(void)
    {
        Parser parser;

        parser.pool = qxc_memory_pool_init(1e3);
        parser.token_buffer = array_create<struct qxc_token>(256);
        parser.itoken = 0;

        return parser;
    }

    static void destroy(Parser* parser)
    {
        // qxc_memory_pool_release(parser->pool);
        array_destroy(&parser->token_buffer);
        parser->itoken = 0;
    }
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

// TODO: remove all this alloc_* functions
static inline struct ExprNode* alloc_empty_expression(Parser* parser)
{
    auto expr = qxc_malloc<struct ExprNode>(parser->pool);
    expr->type = ExprType::Invalid;
    return expr;
}

static inline BlockItemNode* alloc_empty_block_item(Parser* parser)
{
    auto block_item = qxc_malloc<BlockItemNode>(parser->pool);
    block_item->type = BlockItemType::Invalid;
    return block_item;
}

static inline StatementNode* alloc_empty_statement(Parser* parser)
{
    auto statement = qxc_malloc<StatementNode>(parser->pool);
    statement->type = StatementType::Invalid;
    return statement;
}

static inline Declaration* alloc_empty_declaration_node(Parser* parser)
{
    auto decl = qxc_malloc<Declaration>(parser->pool);
    decl->var_name = NULL;
    decl->initializer_expr = NULL;
    return decl;
}

static inline FunctionDecl* alloc_empty_function_decl(Parser* parser)
{
    auto decl = qxc_malloc<FunctionDecl>(parser->pool);
    decl->blist = array_create<BlockItemNode*>(16);
    decl->name = NULL;

    return decl;
}

// static void rewind_token_buffer(Parser* parser, size_t count)
// {
//     while (count > 0 && parser->itoken > 0) {
//         parser->itoken--;
//         count--;
//     }
// }

static struct qxc_token* pop_next_token(Parser* parser)
{
    if (parser->itoken < parser->token_buffer.length) {
        return array_at(&parser->token_buffer, parser->itoken++);
    }
    else {
        return NULL;
    }
}

static struct qxc_token* peek_next_token(Parser* parser)
{
    if (parser->itoken < parser->token_buffer.length) {
        return array_at(&parser->token_buffer, parser->itoken);
    }
    else {
        return NULL;
    }
}

static struct qxc_token* qxc_parser_expect_token_type(
    Parser* parser, enum qxc_token_type expected_token_type)
{
    struct qxc_token* next_token = pop_next_token(parser);
    EXPECT_(next_token && next_token->type == expected_token_type);
    return next_token;
}

static struct qxc_token* qxc_parser_expect_keyword(Parser* parser,
                                                   Keyword expected_keyword)
{
    struct qxc_token* next_token = pop_next_token(parser);

    EXPECT(next_token && next_token->type == KEYWORD_TOKEN, "Expected keyword token");
    EXPECT(next_token->keyword == expected_keyword, "Unexpected keyword token: %s",
           qxc_keyword_to_str(next_token->keyword));

    return next_token;
}

static struct qxc_token* qxc_parser_expect_identifier(Parser* parser,
                                                      const char* expected_identifier)
{
    struct qxc_token* next_token = pop_next_token(parser);

    EXPECT(next_token && next_token->type == IDENTIFIER_TOKEN &&
               strs_are_equal(next_token->name, expected_identifier),
           "Expected identifier");

    return next_token;
}

// static struct qxc_token* qxc_parser_expect_operator(Parser* parser,
//                                                     Operator expected_op)
// {
//     struct qxc_token* next_token = pop_next_token(parser);
//
//     EXPECT(
//         next_token && next_token->type == OPERATOR_TOKEN && next_token->op ==
//         expected_op, "Expected operator");
//
//     return next_token;
// }

static struct ExprNode* qxc_parse_expression(Parser*);

static struct ExprNode* qxc_parse_factor(Parser* parser)
{
    auto factor = qxc_malloc<struct ExprNode>(parser->pool);

    struct qxc_token* next_token = pop_next_token(parser);
    EXPECT_(next_token);

    switch (next_token->type) {
        case INTEGER_LITERAL_TOKEN:
            debug_print("parsed integer literal factor: %ld",
                        next_token->int_literal_value);
            factor->type = ExprType::IntLiteral;
            factor->literal = next_token->int_literal_value;
            break;

        case OPERATOR_TOKEN:
            EXPECT(qxc_operator_can_be_unary(next_token->op),
                   "non-unary operator in unary operator context: %s",
                   qxc_operator_to_str(next_token->op));
            factor->type = ExprType::UnaryOp;
            factor->unop_expr.op = next_token->op;
            factor->unop_expr.child_expr = qxc_parse_factor(parser);
            EXPECT(factor->unop_expr.child_expr,
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
            factor->type = ExprType::VariableRef;
            factor->referenced_var_name = next_token->name;
            break;

        default:
            return NULL;
    }

    return factor;
}

// TODO: clean this up and handle all errors with informative print statements
static int binop_precedence(Operator op)
{
    switch (op) {
        case Operator::Minus:
            return 12;
        case Operator::Plus:
            return 12;
        case Operator::Divide:
            return 13;
        case Operator::Multiply:
            return 13;
        case Operator::LogicalAND:
            return 5;
        case Operator::LogicalOR:
            return 4;
        case Operator::EqualTo:
            return 9;
        case Operator::NotEqualTo:
            return 9;
        case Operator::Colon:
            return 3;
        case Operator::QuestionMark:
            return 3;
        case Operator::LessThan:
            return 10;
        case Operator::LessThanOrEqualTo:
            return 10;
        case Operator::GreaterThan:
            return 10;
        case Operator::GreaterThanOrEqualTo:
            return 10;
        case Operator::Assignment:
            return 2;
        case Operator::Comma:
            return 1;
        case Operator::BitwiseOR:
            return 6;
        case Operator::BitwiseAND:
            return 8;
        case Operator::BitwiseXOR:
            return 7;
        case Operator::BitShiftLeft:
            return 11;
        case Operator::BitShiftRight:
            return 11;
        case Operator::Percent:
            return 13;
        case Operator::Invalid:
            return -999;
        default:
            QXC_UNREACHABLE();
    }
}

static bool binop_valid_for_logical_or_expr_or_below(Operator op)
{
    return binop_precedence(op) > 3;
}

// remember, if things get wonky you can re-write the parser explicitely for each
// production rule in the grammar
static struct ExprNode* qxc_parse_logical_or_expr_(Parser* parser, ExprNode* left_factor,
                                                   int min_precedence)
{
    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    while (next_token->type == OPERATOR_TOKEN &&
           binop_valid_for_logical_or_expr_or_below(next_token->op)) {
        EXPECT(next_token->op != Operator::Assignment,
               "assignment operator in invalid place!");
        const int next_op_precedence = binop_precedence(next_token->op);

        if (next_op_precedence <= min_precedence) {
            break;
        }

        next_token = pop_next_token(parser);
        struct ExprNode* right_expr = qxc_parse_logical_or_expr_(
            parser, qxc_parse_factor(parser), next_op_precedence);
        EXPECT_(right_expr);

        struct ExprNode* new_expr = alloc_empty_expression(parser);
        new_expr->type = ExprType::BinaryOp;
        new_expr->binop_expr.op = next_token->op;
        new_expr->binop_expr.left_expr = left_factor;
        new_expr->binop_expr.right_expr = right_expr;

        left_factor = new_expr;

        next_token = peek_next_token(parser);
        EXPECT_(next_token);
    }

    return left_factor;
}

static struct ExprNode* qxc_parse_logical_or_expr(Parser* parser,
                                                  struct ExprNode* left_factor)
{
    return qxc_parse_logical_or_expr_(parser, left_factor, -1);
}

static struct ExprNode* qxc_parse_conditional_expression(Parser* parser,
                                                         struct ExprNode* left_factor)
{
    struct ExprNode* lor_expr = qxc_parse_logical_or_expr(parser, left_factor);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    if (next_token->op == Operator::QuestionMark) {
        (void)pop_next_token(parser);

        struct ExprNode* ternary_expr = alloc_empty_expression(parser);
        ternary_expr->type = ExprType::Conditional;
        ternary_expr->cond_expr.conditional_expr = lor_expr;
        ternary_expr->cond_expr.if_expr = qxc_parse_expression(parser);
        (void)pop_next_token(parser);
        ternary_expr->cond_expr.else_expr =
            qxc_parse_conditional_expression(parser, qxc_parse_factor(parser));

        return ternary_expr;
    }
    else {
        return lor_expr;
    }
}

static struct ExprNode* qxc_parse_expression(Parser* parser)
{
    struct ExprNode* left_factor = qxc_parse_factor(parser);
    EXPECT_(left_factor);

    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT_(next_token);

    // assignment operator is right-associative, so special treatment here
    if (next_token->op == Operator::Assignment) {
        EXPECT(left_factor->type == ExprType::VariableRef,
               "left hand side of assignment operator must be a variable reference!");
        (void)pop_next_token(parser);

        struct ExprNode* assign_expr = alloc_empty_expression(parser);
        assign_expr->type = ExprType::BinaryOp;
        assign_expr->binop_expr.op = Operator::Assignment;
        assign_expr->binop_expr.left_expr = left_factor;
        assign_expr->binop_expr.right_expr = qxc_parse_expression(parser);

        return assign_expr;
    }
    else {
        return qxc_parse_conditional_expression(parser, left_factor);
    }
}

static Declaration* qxc_parse_declaration(Parser* parser)
{
    struct qxc_token* next_token = pop_next_token(parser);  // pop off int keyword
    assert(next_token->type == KEYWORD_TOKEN && next_token->keyword == Keyword::Int);

    Declaration* new_declaration = alloc_empty_declaration_node(parser);

    next_token = pop_next_token(parser);  // pop off identifier
    EXPECT(next_token && next_token->type == IDENTIFIER_TOKEN,
           "Invalid identifier found for variable declaration name");

    debug_print("parsing declaration of int var: %s", next_token->name);

    const size_t id_len = strlen(next_token->name) + 1;  // includes \0 terminator
    new_declaration->var_name = qxc_malloc_str(parser->pool, id_len);
    memcpy(new_declaration->var_name, next_token->name, id_len);

    next_token = peek_next_token(parser);

    if (next_token && next_token->type == OPERATOR_TOKEN &&
        next_token->op == Operator::Assignment) {
        pop_next_token(parser);  // pop off '='
        new_declaration->initializer_expr = qxc_parse_expression(parser);
        EXPECT(new_declaration->initializer_expr, "Failed to parse variable initializer");
    }

    EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
           "Missing semicolon at end of declaration");

    return new_declaration;
}

static BlockItemNode* qxc_parse_block_item(Parser* parser);

static StatementNode* qxc_parse_statement(Parser* parser)
{
    struct qxc_token* next_token = peek_next_token(parser);

    StatementNode* statement = alloc_empty_statement(parser);

    if (next_token->type == KEYWORD_TOKEN && next_token->keyword == Keyword::Return) {
        pop_next_token(parser);  // pop off return keyword
        statement->type = StatementType::Return;
        statement->return_expr = qxc_parse_expression(parser);
        EXPECT(statement->return_expr, "Expression parsing failed");

        EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
               "Missing semicolon at end of statement");
    }
    else if (next_token->type == KEYWORD_TOKEN && next_token->keyword == Keyword::If) {
        (void)pop_next_token(parser);  // pop off 'if' keyword
        statement->type = StatementType::IfElse;

        IfElseStatement* ifelse_stmt = qxc_malloc<IfElseStatement>(parser->pool);
        ifelse_stmt->conditional_expr = qxc_parse_expression(parser);
        EXPECT_(ifelse_stmt->conditional_expr);
        ifelse_stmt->if_branch_statement = qxc_parse_statement(parser);
        EXPECT_(ifelse_stmt->if_branch_statement);
        statement->ifelse_statement = ifelse_stmt;

        next_token = peek_next_token(parser);

        if (next_token && next_token->type == KEYWORD_TOKEN &&
            next_token->keyword == Keyword::Else) {
            (void)pop_next_token(parser);  // pop off 'else' keyword
            ifelse_stmt->else_branch_statement = qxc_parse_statement(parser);
            EXPECT_(ifelse_stmt->else_branch_statement);
        }
    }
    else if (next_token->type == OPEN_BRACE_TOKEN) {
        (void)pop_next_token(parser);  // pop off 'if' keyword
        statement->type = StatementType::Compound;

        // TODO: abstract out to parse_block_item_list
        statement->compound_statement_block_items = array_create<BlockItemNode*>(4);
        while (peek_next_token(parser)->type != CLOSE_BRACE_TOKEN) {
            BlockItemNode* next_block_item = qxc_parse_block_item(parser);
            EXPECT(next_block_item, "Failed to parse block item in function: main");
            array_extend(&statement->compound_statement_block_items, next_block_item);
            debug_print("successfully parsed block item");
        }

        EXPECT(qxc_parser_expect_token_type(parser, CLOSE_BRACE_TOKEN),
               "Missing closing brace at end of compound statement");
    }
    else {
        // fallthrough, so attempt to parse standalone expression statement
        // for now the only meaningful stand-alone expression is variable assignment
        // i.e. a = 2;
        debug_print("attempting to parse standalone expression");
        struct ExprNode* standalone_expression = qxc_parse_expression(parser);

        EXPECT(standalone_expression, "Failed to parse standalone expression");

        statement->type = StatementType::StandAloneExpr;
        statement->standalone_expr = standalone_expression;

        EXPECT(qxc_parser_expect_token_type(parser, SEMICOLON_TOKEN),
               "Missing semicolon at end of statement");
    }

    return statement;
}

static BlockItemNode* qxc_parse_block_item(Parser* parser)
{
    struct qxc_token* next_token = peek_next_token(parser);
    EXPECT(next_token, "Expected another block item here!");

    BlockItemNode* block_item = alloc_empty_block_item(parser);

    if (next_token->type == KEYWORD_TOKEN && next_token->keyword == Keyword::Int) {
        block_item->type = BlockItemType::Declaration;
        block_item->declaration = qxc_parse_declaration(parser);
        EXPECT_(block_item->declaration);
    }
    else {
        block_item->type = BlockItemType::Statement;
        block_item->statement = qxc_parse_statement(parser);
        EXPECT_(block_item->statement);
    }

    return block_item;
}

static FunctionDecl* qxc_parse_function_decl(Parser* parser)
{
    EXPECT(qxc_parser_expect_keyword(parser, Keyword::Int),
           "Invalid main type signature, must return int.");

    EXPECT(qxc_parser_expect_identifier(parser, "main"), "Invalid main function name");

    EXPECT(qxc_parser_expect_token_type(parser, OPEN_PAREN_TOKEN),
           "Missing open parenthesis");

    // arg parsing would go here, if we allowed function arguments yet

    EXPECT(qxc_parser_expect_token_type(parser, CLOSE_PAREN_TOKEN),
           "Missing close parenthesis");

    EXPECT(qxc_parser_expect_token_type(parser, OPEN_BRACE_TOKEN),
           "Missing open brace token");

    FunctionDecl* decl = alloc_empty_function_decl(parser);
    decl->name = "main";

    while (peek_next_token(parser)->type != CLOSE_BRACE_TOKEN) {
        BlockItemNode* next_block_item = qxc_parse_block_item(parser);
        EXPECT(next_block_item, "Failed to parse block item in function: main");
        array_extend(&decl->blist, next_block_item);
        debug_print("successfully parsed block item");
    }

    EXPECT(qxc_parser_expect_token_type(parser, CLOSE_BRACE_TOKEN),
           "Missing close brace token at end of function: main");

    return decl;
}

Program* qxc_parse(const char* filepath)
{
    Parser parser = Parser::create();
    defer { Parser::destroy(&parser); };

    if (qxc_tokenize(&parser.token_buffer, filepath) != 0) {
        return NULL;
    }

    FunctionDecl* main_decl = qxc_parse_function_decl(&parser);

    EXPECT(main_decl, "Failed to parse main function declaration");

    auto program = qxc_malloc<Program>(parser.pool);
    program->main_decl = main_decl;
    program->pool = parser.pool;

    return program;
}
