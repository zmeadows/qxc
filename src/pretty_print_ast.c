#include "pretty_print_ast.h"

#include <stdio.h>

static size_t indent_level;

#define PPRINT(...)                                 \
    do {                                            \
        for (size_t i = 0; i < indent_level; i++) { \
            printf("  ");                           \
        }                                           \
        printf(__VA_ARGS__);                        \
    } while (0)

static void print_expression(struct qxc_ast_expression_node* node)
{
    switch (node->type) {
        case qxc_int_literal_expr:
            PPRINT("Int<%d>\n", node->int_literal_value);
            break;
        case qxc_unary_op_expr:
            PPRINT("UnaryOp<%c>:\n", qxc_unary_op_to_char(node->op));
            indent_level++;
            print_expression(node->child_expr);
            indent_level--;
            break;
        default:
            printf("\nunrecognized expression type\n");
            return;
    }
}

static void print_statement(struct qxc_ast_statement_node* node)
{
    switch (node->type) {
        case qxc_return_statement:
            PPRINT("Return:\n");
            indent_level++;
            print_expression(node->expr);
            indent_level--;
            break;

        default:
            printf("\nunrecognized statement type\n");
            return;
    }
}

static void print_function_decl(struct qxc_ast_function_decl_node* node)
{
    PPRINT("FUNC NAME: %s\n", node->name);
    indent_level++;
    PPRINT("FUNC RETURN TYPE: Int\n");
    PPRINT("PARAMS: ()\n");
    PPRINT("BODY:\n");
    indent_level++;
    print_statement(node->statement);
    indent_level -= 2;
}

void print_program(struct qxc_program* program)
{
    indent_level = 0;
    print_function_decl(program->main_decl);
}
