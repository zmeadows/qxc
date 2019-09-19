#include "codegen.h"

#include <stdarg.h>
#include <stdio.h>

struct qxc_codegen {
    FILE* asm_output;
    size_t indent_level;
};

static void spit(struct qxc_codegen* gen, const char* fmt, ...)
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

static bool generate_expression_asm(struct qxc_codegen* gen, struct qxc_ast_expression_node* expr)
{
    switch (expr->type) {
        case qxc_int_literal_expr:
            spit(gen, "mov rdi, %d", expr->int_literal_value);
            return true;
        case qxc_unary_op_expr:
            generate_expression_asm(gen, expr->child_expr);
            switch (expr->op) {
                case qxc_negation_unary_op:
                    spit(gen, "neg rdi");
                    break;
                case qxc_bitwise_complement_unary_op:
                    spit(gen, "not rdi");
                    break;
                case qxc_logical_negation_unary_op:
                    spit(gen, "cmp rdi, 0");
                    spit(gen, "mov rdi, 0");
                    spit(gen, "sete dil");
                    break;
                default:
                    return false;
            }
            return true;
    }
    return false;
}

static bool generate_statement_asm(struct qxc_codegen* gen, struct qxc_ast_statement_node* node)
{
    switch (node->type) {
        case qxc_return_statement:
            spit(gen, "mov rax, 60");  // syscall for exit
            generate_expression_asm(gen, node->expr);
            spit(gen, "syscall");
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
    spit(&gen, "global _start");
    spit(&gen, "section .text");
    gen.indent_level--;
    spit(&gen, "_start:");
    gen.indent_level++;
    generate_statement_asm(&gen, program->main_decl->statement);
    gen.indent_level--;

    fclose(gen.asm_output);
    return true;
}
