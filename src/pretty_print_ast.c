#include "pretty_print_ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static size_t indent_level;

#define PPRINT(...)                                 \
    do {                                            \
        for (size_t i = 0; i < indent_level; i++) { \
            printf("| ");                           \
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
        case VARIABLE_REFERENCE_EXPR:
            PPRINT("VariableRef<%s>\n", node->referenced_var_name);
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
            return;

        case EXPRESSION_STATEMENT:
            PPRINT("StandaloneExpr:\n");
            indent_level++;
            print_expression(statement->standalone_expr);
            indent_level--;
            return;

        case CONDITIONAL_STATEMENT:
            if (statement->else_branch_statement != NULL) {
                PPRINT("IfElseStatement:\n");
            }
            else {
                PPRINT("IfStatement:\n");
            }
            indent_level++;

            PPRINT("Condition:\n");
            indent_level++;
            print_expression(statement->conditional_expr);
            indent_level--;
            PPRINT("IfBranch:\n");
            indent_level++;
            print_statement(statement->if_branch_statement);
            indent_level--;

            if (statement->else_branch_statement != NULL) {
                PPRINT("ElseBranch:\n");
                indent_level++;
                print_statement(statement->else_branch_statement);
                indent_level--;
            }

            indent_level--;
            return;

        default:
            printf("\nunrecognized statement type! update pretty_print_ast.c!\n");
            return;
    }
}

static void print_declaration(struct qxc_ast_declaration_node* declaration)
{
    PPRINT("Declaration<%s>:\n", declaration->var_name);
    if (declaration->initializer_expr) {
        indent_level++;
        print_expression(declaration->initializer_expr);
        indent_level--;
    }
}

static void print_block_item(struct qxc_ast_block_item_node* block_item)
{
    if (block_item->type == STATEMENT_BLOCK_ITEM) {
        print_statement(block_item->statement);
    }
    else if (block_item->type == DECLARATION_BLOCK_ITEM) {
        print_declaration(block_item->declaration);
    }
    else {
        assert(block_item->type == INVALID_BLOCK_ITEM);
        fprintf(stderr, "invalid block item encountered");
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

    struct qxc_block_item_list* b = node->blist;
    while (b->node != NULL) {
        print_block_item(b->node);
        b = b->next_node;
    }
    indent_level -= 2;
}

void print_program(struct qxc_program* program)
{
    indent_level = 0;
    print_function_decl(program->main_decl);
    printf("\n");
}
