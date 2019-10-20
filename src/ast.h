#pragma once

#include "allocator.h"
#include "prelude.h"
#include "token.h"

// --------------------------------------------------------------------------------

enum qxc_expression_type {
    INT_LITERAL_EXPR,
    UNARY_OP_EXPR,
    BINARY_OP_EXPR,
    VARIABLE_REFERENCE_EXPR,
    CONDITIONAL_EXPR,
    INVALID_EXPR
};

// TODO: rename to _expr, make static alloc functions
struct ast_unop_node {
    enum qxc_operator op;
    struct qxc_ast_expression_node* child_expr;
};

struct ast_binop_node {
    enum qxc_operator op;
    struct qxc_ast_expression_node* left_expr;
    struct qxc_ast_expression_node* right_expr;
};

struct ast_conditional_node {
    struct qxc_ast_expression_node* conditional_expr;
    struct qxc_ast_expression_node* if_expr;
    struct qxc_ast_expression_node* else_expr;
};

struct qxc_ast_expression_node {
    enum qxc_expression_type type;

    union {
        long literal;
        struct ast_unop_node unop_expr;
        struct ast_binop_node binop_expr;
        struct ast_conditional_node cond_expr;
        const char* referenced_var_name;
    };
};

// --------------------------------------------------------------------------------

enum qxc_statement_type {
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
    CONDITIONAL_STATEMENT,
    COMPOUND_STATEMENT,
    INVALID_STATEMENT
};

struct ast_ifelse_statement {
    struct qxc_ast_expression_node* conditional_expr;
    struct qxc_ast_statement_node* if_branch_statement;
    struct qxc_ast_statement_node* else_branch_statement;  // optional, may be NULL
};

struct qxc_ast_statement_node {
    enum qxc_statement_type type;

    union {
        struct qxc_ast_expression_node* return_expr;
        struct ast_ifelse_statement ifelse_statement;
        struct qxc_block_item_list* compound_statement_block_items;
        struct qxc_ast_expression_node* standalone_expr;
    };
};

// --------------------------------------------------------------------------------

struct qxc_ast_declaration_node {
    char* var_name;
    struct qxc_ast_expression_node* initializer_expr;
};

// --------------------------------------------------------------------------------

enum qxc_block_item_type {
    STATEMENT_BLOCK_ITEM,
    DECLARATION_BLOCK_ITEM,
    INVALID_BLOCK_ITEM
};

struct qxc_ast_block_item_node {
    enum qxc_block_item_type type;
    union {
        struct qxc_ast_statement_node* statement;
        struct qxc_ast_declaration_node* declaration;
    };
};

struct qxc_block_item_list {
    struct qxc_ast_block_item_node* node;
    struct qxc_block_item_list* next_node;
};

// --------------------------------------------------------------------------------

struct qxc_ast_function_decl_node {
    const char* name;
    struct qxc_block_item_list* blist;
};

// --------------------------------------------------------------------------------

struct qxc_program {  // program
    struct qxc_ast_function_decl_node* main_decl;
    struct qxc_memory_pool* ast_memory_pool;
};

struct qxc_program* qxc_parse(const char* filepath);

