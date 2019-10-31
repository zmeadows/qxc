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

void print_expression(struct ExprNode* node)
{
    switch (node->type) {
        case ExprType::IntLiteral:
            PPRINT("Int<%ld>\n", node->literal);
            break;

        case ExprType::UnaryOp:
            PPRINT("UnaryOp<%s>:\n", operator_to_str(node->unop_expr.op));
            indent_level++;
            print_expression(node->unop_expr.child_expr);
            indent_level--;
            break;

        case ExprType::BinaryOp:
            PPRINT("BinaryOp<%s>:\n", operator_to_str(node->binop_expr.op));
            indent_level++;
            print_expression(node->binop_expr.left_expr);
            print_expression(node->binop_expr.right_expr);
            indent_level--;
            break;

        case ExprType::VariableRef:
            PPRINT("VariableRef<%s>\n", node->referenced_var_name);
            break;

        case ExprType::Conditional:
            PPRINT("TernaryConditional\n");
            indent_level++;
            PPRINT("Condition:\n");
            indent_level++;
            print_expression(node->cond_expr.conditional_expr);
            indent_level--;
            PPRINT("IfExpr:\n");
            indent_level++;
            print_expression(node->cond_expr.if_expr);
            indent_level--;
            PPRINT("ElseExpr:\n");
            indent_level++;
            print_expression(node->cond_expr.else_expr);
            indent_level--;
            indent_level--;
            break;

        case ExprType::Invalid:
            PPRINT("InvalidExpr");
            break;

        default:
            printf("\nunrecognized expression type\n");
            return;
    }
}

static void print_block_item(BlockItemNode* block_item);

static void print_statement(StatementNode* statement)
{
    switch (statement->type) {
        case StatementType::Return:
            PPRINT("Return:\n");
            indent_level++;
            print_expression(statement->return_expr);
            indent_level--;
            return;

        case StatementType::StandAloneExpr:
            PPRINT("StandaloneExpr:\n");
            indent_level++;
            print_expression(statement->standalone_expr);
            indent_level--;
            return;

        case StatementType::IfElse: {
            IfElseStatement* ifelse_stmt = statement->ifelse_statement;

            if (ifelse_stmt->else_branch_statement != nullptr) {
                PPRINT("IfElseStatement:\n");
            }
            else {
                PPRINT("IfStatement:\n");
            }
            indent_level++;

            PPRINT("Condition:\n");
            indent_level++;
            print_expression(ifelse_stmt->conditional_expr);
            indent_level--;
            PPRINT("IfBranch:\n");
            indent_level++;
            print_statement(ifelse_stmt->if_branch_statement);
            indent_level--;

            if (ifelse_stmt->else_branch_statement != nullptr) {
                PPRINT("ElseBranch:\n");
                indent_level++;
                print_statement(ifelse_stmt->else_branch_statement);
                indent_level--;
            }

            indent_level--;
            return;
        }

        case StatementType::Compound: {
            PPRINT("CompoundStatement:\n");
            indent_level++;

            for (BlockItemNode* b : statement->compound_statement_block_items) {
                print_block_item(b);
            }

            indent_level--;
            return;
        }

        default:
            printf("\nunrecognized statement type! update pretty_print_ast.c!\n");
            return;
    }
}

static void print_declaration(Declaration* declaration)
{
    PPRINT("Declaration<%s>:\n", declaration->var_name);
    if (declaration->initializer_expr) {
        indent_level++;
        print_expression(declaration->initializer_expr);
        indent_level--;
    }
}

static void print_block_item(BlockItemNode* block_item)
{
    if (block_item->type == BlockItemType::Statement) {
        print_statement(block_item->statement);
    }
    else if (block_item->type == BlockItemType::Declaration) {
        print_declaration(block_item->declaration);
    }
    else {
        assert(block_item->type == BlockItemType::Invalid);
        fprintf(stderr, "invalid block item encountered");
    }
}

static void print_function_decl(FunctionDecl* node)
{
    PPRINT("FUNC NAME: %s\n", node->name);
    indent_level++;
    PPRINT("FUNC RETURN TYPE: Int\n");
    PPRINT("PARAMS: ()\n");
    PPRINT("BODY:\n");
    indent_level++;

    for (BlockItemNode* b : node->blist) {
        print_block_item(b);
    }

    indent_level -= 2;
}

void print_program(Program* program)
{
    indent_level = 0;
    print_function_decl(program->main_decl);
    printf("\n");
}
