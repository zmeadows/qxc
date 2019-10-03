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

void print_expression(struct qxc_ast_expression_node* node)
{
    switch (node->type) {
        case INT_LITERAL_EXPR:
            PPRINT("Int<%ld>\n", node->literal);
            break;
        case UNARY_OP_EXPR:
            PPRINT("UnaryOp<%s>:\n", qxc_operator_to_str(node->unop));
            indent_level++;
            print_expression(node->unary_expr);
            indent_level--;
            break;
        case BINARY_OP_EXPR:
            PPRINT("BinaryOp<%s>:\n", qxc_operator_to_str(node->binop));
            indent_level++;
            print_expression(node->left_expr);
            print_expression(node->right_expr);
            indent_level--;
            break;
        case ASSIGNMENT_EXPR:
            PPRINT("Assignment<%s>:\n", node->assignee_var_name);
            indent_level++;
            print_expression(node->assignment_expr);
            indent_level--;
            break;
        case VARIABLE_REFERENCE_EXPR:
            PPRINT("VariableRef<%s>:\n", node->referenced_var_name);
            break;
        case INVALID_EXPR:
            PPRINT("InvalidExpr");
            break;
        default:
            printf("\nunrecognized expression type\n");
            return;
    }
}

static void print_statement(struct qxc_ast_statement_node* statement)
{
    switch (statement->type) {
        case RETURN_STATEMENT:
            PPRINT("Return:\n");
            indent_level++;
            print_expression(statement->return_expr);
            indent_level--;
            break;

        case DECLARATION_STATEMENT:
            PPRINT("Declaration<%s>:\n", statement->var_name);
            indent_level++;
            if (statement->initializer_expr) {
                print_expression(statement->initializer_expr);
            }
            indent_level--;
            break;

        case EXPRESSION_STATEMENT:
            PPRINT("StandaloneExpr:\n");
            indent_level++;
            print_expression(statement->standalone_expr);
            indent_level--;
            break;

        default:
            printf("\nunrecognized statement type! update pretty_print_ast.c!\n");
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

    struct qxc_statement_list* s = node->slist;
    while (s->node != NULL) {
        print_statement(s->node);
        s = s->next_node;
    }
    indent_level -= 2;
}

void print_program(struct qxc_program* program)
{
    indent_level = 0;
    print_function_decl(program->main_decl);
}
