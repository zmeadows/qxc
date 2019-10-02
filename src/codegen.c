#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>

struct qxc_codegen {
    FILE* asm_output;
    size_t indent_level;

    size_t logical_or_counter;
    char logical_or_jump_label[256];
    char logical_or_end_label[256];

    size_t logical_and_counter;
    char logical_and_jump_label[256];
    char logical_and_end_label[256];
};

static void qxc_codegen_increment_logical_or_count(struct qxc_codegen* gen)
{
    // TODO: overflow protection
    gen->logical_or_counter++;
    sprintf(gen->logical_or_jump_label, "_LogicalOr_SndClause_%zu",
            gen->logical_or_counter);
    sprintf(gen->logical_or_end_label, "_LogicalOr_End_%zu", gen->logical_or_counter);
}

static void qxc_codegen_increment_logical_and_count(struct qxc_codegen* gen)
{
    // TODO: overflow protection
    gen->logical_and_counter++;
    sprintf(gen->logical_and_jump_label, "_LogicalAnd_SndClause_%zu",
            gen->logical_and_counter);
    sprintf(gen->logical_and_end_label, "_LogicalAnd_End_%zu", gen->logical_and_counter);
}

static void emit(struct qxc_codegen* gen, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    for (size_t i = 0; i < gen->indent_level; i++) {
        fprintf(gen->asm_output, "  ");
    }
    vfprintf(gen->asm_output, fmt, args);
    va_end(args);
    fprintf(gen->asm_output, "\n");
}

static bool operator_is_short_circuiting(enum qxc_operator op)
{
    switch (op) {
        case LOGICAL_OR_OP:
            return true;
        case LOGICAL_AND_OP:
            return true;
        default:
            return false;
    }
}

static void generate_expression_asm(struct qxc_codegen* gen,
                                    struct qxc_ast_expression_node* expr)
{
    switch (expr->type) {
        case INT_LITERAL_EXPR:
            emit(gen, "mov rax, %d", expr->literal);
            return;

        case UNARY_OP_EXPR:
            generate_expression_asm(gen, expr->unary_expr);

            switch (expr->unop) {
                case MINUS_OP:
                    emit(gen, "neg rax");
                    break;
                case COMPLEMENT_OP:
                    emit(gen, "not rax");
                    break;
                case NEGATION_OP:
                    emit(gen, "cmp rax, 0");
                    emit(gen, "mov rax, 0");
                    emit(gen, "sete al");
                    break;
                default:
                    break;
            }

            return;

        case BINARY_OP_EXPR:
            if (operator_is_short_circuiting(expr->binop)) {
                generate_expression_asm(gen, expr->left_expr);

                switch (expr->binop) {
                    case LOGICAL_OR_OP:
                        emit(gen, "cmp rax, 0");
                        emit(gen, "je %s", gen->logical_or_jump_label);
                        emit(gen, "mov rax, 1");
                        emit(gen, "jmp %s", gen->logical_or_end_label);
                        emit(gen, "%s:", gen->logical_or_jump_label);
                        generate_expression_asm(gen, expr->right_expr);
                        emit(gen, "cmp rax, 0");
                        emit(gen, "mov rax, 0");
                        emit(gen, "setne al");
                        emit(gen, "%s:", gen->logical_or_end_label);

                        qxc_codegen_increment_logical_or_count(gen);
                        break;

                    case LOGICAL_AND_OP:
                        emit(gen, "cmp rax, 0");
                        emit(gen, "jne %s", gen->logical_and_jump_label);
                        emit(gen, "jmp %s", gen->logical_and_end_label);
                        emit(gen, "%s:", gen->logical_and_jump_label);
                        generate_expression_asm(gen, expr->right_expr);
                        emit(gen, "cmp rax, 0");
                        emit(gen, "mov rax, 0");
                        emit(gen, "setne al");
                        emit(gen, "%s:", gen->logical_and_end_label);

                        qxc_codegen_increment_logical_and_count(gen);
                        break;
                    default:
                        break;
                        // TODO: error handling
                }
            }
            else {
                // put left hand operand in rax, right hand operand in rbx
                generate_expression_asm(gen, expr->right_expr);
                emit(gen, "push rax");
                generate_expression_asm(gen, expr->left_expr);
                emit(gen, "pop rbx");

                switch (expr->binop) {
                    case PLUS_OP:
                        emit(gen, "add rax, rbx");
                        break;
                    case MINUS_OP:
                        emit(gen, "sub rax, rbx");
                        break;
                    case DIVIDE_OP:
                        emit(gen, "xor rdx, rdx");
                        emit(gen, "idiv rbx");
                        break;
                    case MULTIPLY_OP:
                        emit(gen, "imul rbx");
                        break;
                    case EQUAL_TO_OP:
                        emit(gen, "cmp rax, rbx");
                        emit(gen, "mov rax, 0");
                        emit(gen, "sete al");
                        break;
                    case NOT_EQUAL_TO_OP:
                        emit(gen, "cmp rax, rbx");
                        emit(gen, "mov rax, 0");
                        emit(gen, "setne al");
                        break;
                    case LESS_THAN_OP:
                        emit(gen, "cmp rax, rbx");
                        emit(gen, "mov rax, 0");
                        emit(gen, "setl al");
                        break;
                    case LESS_THAN_OR_EQUAL_TO_OP:
                        emit(gen, "cmp rax, rbx");
                        emit(gen, "mov rax, 0");
                        emit(gen, "setle al");
                        break;
                    case GREATER_THAN_OP:
                        emit(gen, "cmp rax, rbx");
                        emit(gen, "mov rax, 0");
                        emit(gen, "setg al");
                        break;
                    case GREATER_THAN_OR_EQUAL_TO_OP:
                        emit(gen, "cmp rax, rbx");
                        emit(gen, "mov rax, 0");
                        emit(gen, "setge al");
                        break;
                    default:
                        // TODO: error handling
                        break;
                }
            }

            return;

        default:
            // TODO: error handling
            return;
    }
}

static void generate_statement_asm(struct qxc_codegen* gen,
                                   struct qxc_ast_statement_node* node)
{
    switch (node->type) {
        case RETURN_STATEMENT:
            generate_expression_asm(gen, node->expr);
            emit(gen, "mov rdi, rax");
            emit(gen, "mov rax, 60");  // syscall for exit
            emit(gen, "syscall");
            break;

        default:
            printf("\nunrecognized statement type\n");
            break;
    }
    return;
}

void generate_asm(struct qxc_program* program, const char* output_filepath)
{
    struct qxc_codegen gen;
    gen.indent_level = 0;
    gen.logical_or_counter = 0;
    sprintf(gen.logical_or_jump_label, "_LogicalOr_SndClause_0");
    sprintf(gen.logical_or_end_label, "_LogicalOr_End_0");

    gen.logical_and_counter = 0;
    sprintf(gen.logical_and_jump_label, "_LogicalAnd_SndClause_0");
    sprintf(gen.logical_and_end_label, "_LogicalAnd_End_0");

    gen.asm_output = fopen(output_filepath, "w");

    gen.indent_level++;
    emit(&gen, "global _start");
    emit(&gen, "section .text");
    gen.indent_level--;
    emit(&gen, "_start:");
    gen.indent_level++;

    struct qxc_statement_list* s = program->main_decl->slist;
    while (s->next_node != NULL) {
        generate_statement_asm(&gen, s->node);
        s = s->next_node;
    }
    gen.indent_level--;

    fclose(gen.asm_output);
}
