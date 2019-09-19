#pragma once

#include "token.h"

enum qxc_expr_type { qxc_int_literal_expr, qxc_unary_op_expr };

struct qxc_ast_expression_node;

struct qxc_ast_unary_op_node {
    enum qxc_unary_op op;
    struct qxc_ast_expression_node* expr;
};

struct qxc_ast_expression_node {
    enum qxc_expr_type type;

    union {
        int int_literal_value;
        struct {
            enum qxc_unary_op op;
            struct qxc_ast_expression_node* child_expr;
        };
    };
};

enum qxc_statement_type { qxc_return_statement };

struct qxc_ast_statement_node {
    enum qxc_statement_type type;

    union {
        struct qxc_ast_expression_node* expr;
    };
};

struct qxc_ast_function_decl_node {
    const char* name;
    struct qxc_ast_statement_node* statement;
};

struct qxc_program {  // program
    struct qxc_ast_function_decl_node* main_decl;
};

struct qxc_program* qxc_parse(const char* filepath);

