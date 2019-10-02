#pragma once

#include "prelude.h"
#include "token.h"

// --------------------------------------------------------------------------------

enum qxc_expression_type {
    INT_LITERAL_EXPR,
    UNARY_OP_EXPR,
    BINARY_OP_EXPR,
    ASSIGNMENT_EXPR,
    VARIABLE_REFERENCE_EXPR,
    INVALID_EXPR
};

struct qxc_ast_expression_node {
    enum qxc_expression_type type;

    union {
        long literal;

        struct {
            enum qxc_operator unop;
            struct qxc_ast_expression_node* unary_expr;
        };

        struct {
            enum qxc_operator binop;
            struct qxc_ast_expression_node* left_expr;
            struct qxc_ast_expression_node* right_expr;
        };

        struct {
            const char* assignee_var_name;
            struct qxc_ast_expression_node* assignment_expr;
        };

        const char* referenced_var_name;
    };
};

// --------------------------------------------------------------------------------

enum qxc_statement_type {
    RETURN_STATEMENT,
    DECLARATION_STATEMENT,
    EXPRESSION_STATEMENT,
    INVALID_STATEMENT
};

struct qxc_ast_statement_node {
    enum qxc_statement_type type;

    union {
        struct qxc_ast_expression_node* return_expr;
        struct {
            const char* var_name;
            struct qxc_ast_expression_node* initializer_expr;
        };
        struct qxc_ast_expression_node* standalone_expr;
    };
};

struct qxc_statement_list {
    struct qxc_ast_statement_node* node;
    struct qxc_statement_list* next_node;
};

// --------------------------------------------------------------------------------

struct qxc_ast_function_decl_node {
    const char* name;
    struct qxc_statement_list* slist;
};

// --------------------------------------------------------------------------------

struct qxc_program {  // program
    struct qxc_ast_function_decl_node* main_decl;
};

struct qxc_program* qxc_parse(const char* filepath);

