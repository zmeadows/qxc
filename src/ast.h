#pragma once

#include "prelude.h"
#include "token.h"

// <program> ::= <function>
// <function> ::= "int" <id> "(" ")" "{" <statement> "}"
// <statement> ::= "return" <exp> ";"
// <exp> ::= <term> { ("+" | "-") <term> }
// <term> ::= <factor> { ("*" | "/") <factor> }
// <factor> ::= "(" <exp> ")" | <unary_op> <factor> | <int>

// --------------------------------------------------------------------------------

enum qxc_expression_type {
    INT_LITERAL_EXPR,
    UNARY_OP_EXPR,
    BINARY_OP_EXPR,
    INVALID_EXPR
};

struct qxc_ast_expression_node {
    enum qxc_expression_type type;

    union {
        struct {
            enum qxc_operator binop;
            struct qxc_ast_expression_node* left_expr;
            struct qxc_ast_expression_node* right_expr;
        };

        struct {
            enum qxc_operator unop;
            struct qxc_ast_expression_node* unary_expr;
        };

        int literal;
    };
};

// --------------------------------------------------------------------------------

enum qxc_statement_type { RETURN_STATEMENT };

struct qxc_ast_statement_node {
    enum qxc_statement_type type;

    union {
        struct qxc_ast_expression_node* expr;
    };
};
// --------------------------------------------------------------------------------

struct qxc_ast_function_decl_node {
    const char* name;
    struct qxc_ast_statement_node* statement;
};
// --------------------------------------------------------------------------------

struct qxc_program {  // program
    struct qxc_ast_function_decl_node* main_decl;
};

struct qxc_program* qxc_parse(const char* filepath);

