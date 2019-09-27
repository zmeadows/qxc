#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>

struct qxc_codegen {
    FILE* asm_output;
    size_t indent_level;
};

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

static bool generate_expression_asm(struct qxc_codegen* gen,
                                    struct qxc_ast_expression_node* expr)
{
    switch (expr->type) {
        case INT_LITERAL_EXPR:
            emit(gen, "mov rax, %d", expr->literal);

            return true;

        case UNARY_OP_EXPR:
            generate_expression_asm(gen, expr->unary_expr);
            switch (expr->unop) {
                case qxc_minus_op:
                    emit(gen, "neg rax");
                    break;
                case qxc_complement_op:
                    emit(gen, "not rax");
                    break;
                case qxc_exclamation_op:
                    emit(gen, "cmp rax, 0");
                    emit(gen, "mov rax, 0");
                    emit(gen, "sete al");
                    break;
                default:
                    return false;
            }
            return true;

        case BINARY_OP_EXPR:
            // put left hand operand in rax, right hand operand in rbx
            generate_expression_asm(gen, expr->right_expr);
            emit(gen, "push rax");
            generate_expression_asm(gen, expr->left_expr);
            emit(gen, "pop rbx");

            switch (expr->binop) {
                case qxc_plus_op:
                    emit(gen, "add rax, rbx");
                    break;
                case qxc_minus_op:
                    emit(gen, "sub rax, rbx");
                    break;
                case qxc_divide_op:
                    emit(gen, "xor rdx, rdx");
                    emit(gen, "idiv rbx");
                    break;
                case qxc_multiply_op:
                    emit(gen, "imul rbx");
                    break;
                default:
                    return false;
            }
            return true;

        default:
            return false;
    }
}

static bool generate_statement_asm(struct qxc_codegen* gen,
                                   struct qxc_ast_statement_node* node)
{
    switch (node->type) {
        case RETURN_STATEMENT:
            generate_expression_asm(gen, node->expr);
            emit(gen, "mov rdi, rax");
            emit(gen, "mov rax, 60");  // syscall for exit
            emit(gen, "syscall");
            return true;

        default:
            printf("\nunrecognized statement type\n");
            return false;
    }
}

bool generate_asm(struct qxc_program* program, const char* output_filepath)
{
    struct qxc_codegen gen;
    gen.indent_level = 0;
    gen.asm_output = fopen(output_filepath, "w");

    gen.indent_level++;
    emit(&gen, "global _start");
    emit(&gen, "section .text");
    gen.indent_level--;
    emit(&gen, "_start:");
    gen.indent_level++;
    generate_statement_asm(&gen, program->main_decl->statement);
    gen.indent_level--;

    fclose(gen.asm_output);
    return true;
}
