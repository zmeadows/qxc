#include "codegen.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "flat_map.h"

#define QXC_STACK_OFFSETS_CAPACITY 64
struct qxc_stack_offsets {
    const char* variable_names[QXC_STACK_OFFSETS_CAPACITY];
    int variable_offsets[QXC_STACK_OFFSETS_CAPACITY];
    int stack_index;
    size_t count;
};

static struct qxc_stack_offsets qxc_stack_offsets_new(void)
{
    struct qxc_stack_offsets offsets;
    offsets.count = 0;
    offsets.stack_index = -8;  // base of callers stack frame always saved as first entry
                               // of calee's stack frame, so start with -8 rather than 0
    return offsets;
}

static bool qxc_stack_offsets_contains(struct qxc_stack_offsets* offsets,
                                       const char* name)
{
    const size_t count = offsets->count;
    for (size_t i = 0; i < count; i++) {
        if (strs_are_equal(name, offsets->variable_names[i])) {
            return true;
        }
    }

    return false;
}

static int qxc_stack_offsets_insert(struct qxc_stack_offsets* offsets, const char* name)
{
    const size_t old_count = offsets->count;
    assert(old_count < QXC_STACK_OFFSETS_CAPACITY);
    offsets->variable_names[old_count] = name;
    offsets->variable_offsets[old_count] = offsets->stack_index;
    offsets->count++;
    offsets->stack_index -= 8;
    return 0;
}

static int qxc_stack_offsets_lookup(struct qxc_stack_offsets* offsets, const char* name)
{
    const size_t count = offsets->count;
    for (size_t i = 0; i < count; i++) {
        if (strs_are_equal(name, offsets->variable_names[i])) {
            return offsets->variable_offsets[i];
        }
    }
    exit(EXIT_FAILURE);
}

// static void qxc_stack_offsets_clear(struct qxc_stack_offsets* offsets)
// {
//     for (size_t i = 0; i < offsets->count; i++) {
//         offsets->variable_names[i] = NULL;
//         offsets->variable_offsets[i] = 0;
//     }
//
//     offsets->count = 0;
// }

#define LABEL_COUNTER_NAME_MAX_SIZE 256
struct qxc_codegen {
    FILE* asm_output;
    size_t indent_level;

    size_t logical_or_counter;
    size_t logical_and_counter;
    size_t conditional_expr_counter;
};

#define JUMP_LABEL_MAX_LENGTH 64
struct qxc_jump_label {
    char buffer[JUMP_LABEL_MAX_LENGTH];
};

static void build_logical_or_jump_labels(struct qxc_codegen* gen,
                                         struct qxc_jump_label* snd_label,
                                         struct qxc_jump_label* end_label)
{
    size_t count = ++gen->logical_or_counter;
    qxc_snprintf(snd_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LOR_Snd_%zu", count);
    qxc_snprintf(end_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LOR_End_%zu", count);
}

static void build_logical_and_jump_labels(struct qxc_codegen* gen,
                                          struct qxc_jump_label* snd_label,
                                          struct qxc_jump_label* end_label)
{
    size_t count = ++gen->logical_and_counter;
    qxc_snprintf(snd_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LAND_Snd_%zu", count);
    qxc_snprintf(end_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LAND_End_%zu", count);
}

static void build_conditional_expr_jump_labels(struct qxc_codegen* gen,
                                               struct qxc_jump_label* else_label,
                                               struct qxc_jump_label* post_label)
{
    size_t count = ++gen->conditional_expr_counter;
    qxc_snprintf(else_label->buffer, JUMP_LABEL_MAX_LENGTH, "_CondExpr_Else_%zu", count);
    qxc_snprintf(post_label->buffer, JUMP_LABEL_MAX_LENGTH, "_CondExpr_Post_%zu", count);
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

static void generate_expression_asm(struct qxc_codegen* gen,
                                    struct qxc_stack_offsets* offsets,
                                    struct qxc_ast_expression_node* expr);

static void generate_logical_OR_binop_expression_asm(struct qxc_codegen* gen,
                                                     struct qxc_stack_offsets* offsets,
                                                     struct qxc_ast_expression_node* expr)
{
    struct qxc_jump_label snd_label;
    struct qxc_jump_label end_label;
    build_logical_or_jump_labels(gen, &snd_label, &end_label);

    generate_expression_asm(gen, offsets, expr->left_expr);

    emit(gen, "cmp rax, 0");
    emit(gen, "je %s", snd_label.buffer);
    emit(gen, "mov rax, 1");
    emit(gen, "jmp %s", end_label.buffer);

    emit(gen, "\n%s:", snd_label.buffer);

    generate_expression_asm(gen, offsets, expr->right_expr);
    emit(gen, "cmp rax, 0");
    emit(gen, "mov rax, 0");
    emit(gen, "setne al");

    emit(gen, "\n%s:", end_label.buffer);
}

static void generate_logical_AND_binop_expression_asm(
    struct qxc_codegen* gen, struct qxc_stack_offsets* offsets,
    struct qxc_ast_expression_node* expr)
{
    struct qxc_jump_label snd_label;
    struct qxc_jump_label end_label;
    build_logical_and_jump_labels(gen, &snd_label, &end_label);

    generate_expression_asm(gen, offsets, expr->left_expr);

    emit(gen, "cmp rax, 0");
    emit(gen, "jne %s", snd_label.buffer);
    emit(gen, "jmp %s", end_label.buffer);

    emit(gen, "%s:", snd_label.buffer);

    generate_expression_asm(gen, offsets, expr->right_expr);
    emit(gen, "cmp rax, 0");
    emit(gen, "mov rax, 0");
    emit(gen, "setne al");

    emit(gen, "%s:", end_label.buffer);
}

static void generate_assignment_binop_expression_asm(struct qxc_codegen* gen,
                                                     struct qxc_stack_offsets* offsets,
                                                     struct qxc_ast_expression_node* expr)
{
    assert(expr->left_expr && expr->right_expr);
    assert(expr->left_expr->type == VARIABLE_REFERENCE_EXPR);

    const char* varname = expr->left_expr->referenced_var_name;

    // generate value to be assigned to variable in left_expr
    generate_expression_asm(gen, offsets, expr->right_expr);

    if (!qxc_stack_offsets_contains(offsets, varname)) {
        fprintf(stderr, "attempted to assign value to un-initialized variable: %s\n",
                varname);
        exit(EXIT_FAILURE);
    }

    emit(gen, "mov [rbp + %d], rax", qxc_stack_offsets_lookup(offsets, varname));
}

static void generate_binop_expression_asm(struct qxc_codegen* gen,
                                          struct qxc_stack_offsets* offsets,
                                          struct qxc_ast_expression_node* expr)
{
    assert(expr->type == BINARY_OP_EXPR);

    // separate treatment due to short circuiting
    if (expr->binop == LOGICAL_OR_OP) {
        generate_logical_OR_binop_expression_asm(gen, offsets, expr);
        return;
    }

    // separate treatment due to short circuiting
    if (expr->binop == LOGICAL_AND_OP) {
        generate_logical_AND_binop_expression_asm(gen, offsets, expr);
        return;
    }

    // separate treatment to avoid evaluating left expr
    if (expr->binop == ASSIGNMENT_OP) {
        generate_assignment_binop_expression_asm(gen, offsets, expr);
        return;
    }

    // Now, all remaining binary operations behave similarly.

    // Put left hand operand in rax, right hand operand in rbx
    generate_expression_asm(gen, offsets, expr->right_expr);
    emit(gen, "push rax");
    generate_expression_asm(gen, offsets, expr->left_expr);
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
            fprintf(stderr, "fallthrough to unsupported binary operation!");
            break;
    }
}

static void generate_expression_asm(struct qxc_codegen* gen,
                                    struct qxc_stack_offsets* offsets,
                                    struct qxc_ast_expression_node* expr)
{
    switch (expr->type) {
        case INT_LITERAL_EXPR:
            emit(gen, "mov rax, %d", expr->literal);
            return;

        case UNARY_OP_EXPR:
            generate_expression_asm(gen, offsets, expr->unary_expr);

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
            generate_binop_expression_asm(gen, offsets, expr);
            return;

        case CONDITIONAL_EXPR: {
            struct qxc_jump_label else_label;
            struct qxc_jump_label post_label;
            build_conditional_expr_jump_labels(gen, &else_label, &post_label);

            generate_expression_asm(gen, offsets, expr->conditional_expr);
            emit(gen, "cmp rax, 0");
            emit(gen, "je %s", else_label.buffer);
            generate_expression_asm(gen, offsets, expr->if_expr);
            emit(gen, "jmp %s", post_label.buffer);
            emit(gen, "\n%s:", else_label.buffer);
            generate_expression_asm(gen, offsets, expr->else_expr);
            emit(gen, "\n%s:", post_label.buffer);
            return;
        }

        case VARIABLE_REFERENCE_EXPR:
            if (!qxc_stack_offsets_contains(offsets, expr->referenced_var_name)) {
                fprintf(stderr, "referenced unknown variable: %s\n",
                        expr->referenced_var_name);
                exit(EXIT_FAILURE);
            }
            emit(gen, "mov rax, [rbp + %d]",
                 qxc_stack_offsets_lookup(offsets, expr->referenced_var_name));
            return;

        default:
            // TODO: error handling
            return;
    }
}

static void generate_statement_asm(struct qxc_codegen* gen,
                                   struct qxc_stack_offsets* offsets,
                                   struct qxc_ast_statement_node* statement_node)
{
    switch (statement_node->type) {
        case RETURN_STATEMENT:
            generate_expression_asm(gen, offsets, statement_node->return_expr);
            emit(gen, "mov rdi, rax");
            emit(gen, "mov rax, 60");  // syscall for exit
            emit(gen, "syscall");
            break;

        case EXPRESSION_STATEMENT:
            generate_expression_asm(gen, offsets, statement_node->standalone_expr);
            break;

        case CONDITIONAL_STATEMENT: {
            struct qxc_jump_label else_label;
            struct qxc_jump_label post_label;
            build_conditional_expr_jump_labels(gen, &else_label, &post_label);

            if (statement_node->else_branch_statement != NULL) {
                generate_expression_asm(gen, offsets, statement_node->conditional_expr);
                emit(gen, "cmp rax, 0");
                emit(gen, "je %s", else_label.buffer);
                generate_statement_asm(gen, offsets, statement_node->if_branch_statement);
                emit(gen, "jmp %s", post_label.buffer);
                emit(gen, "\n%s:", else_label.buffer);
                generate_statement_asm(gen, offsets,
                                       statement_node->else_branch_statement);
                emit(gen, "\n%s:", post_label.buffer);
            }
            else {
                generate_expression_asm(gen, offsets, statement_node->conditional_expr);
                emit(gen, "cmp rax, 0");
                emit(gen, "je %s", post_label.buffer);
                generate_statement_asm(gen, offsets, statement_node->if_branch_statement);
                emit(gen, "\n%s:", post_label.buffer);
            }
            break;
        }

        default:
            // printf("\nunrecognized statement type\n");
            break;
    }
    return;
}

static void generate_declaration_asm(struct qxc_codegen* gen,
                                     struct qxc_stack_offsets* offsets,
                                     struct qxc_ast_declaration_node* declaration)
{
    if (qxc_stack_offsets_contains(offsets, declaration->var_name)) {
        fprintf(stderr, "variable declared twice: %s\n", declaration->var_name);
        exit(EXIT_FAILURE);
    }

    if (declaration->initializer_expr) {
        // push initializer expression value onto stack
        generate_expression_asm(gen, offsets, declaration->initializer_expr);
        emit(gen, "push rax");
    }
    else {
        // push uninitialized value onto stack.
        emit(gen, "push 0");
    }

    // associate newly pushed stack value with the corresponding new variable
    qxc_stack_offsets_insert(offsets, declaration->var_name);
}

static void generate_block_item_asm(struct qxc_codegen* gen,
                                    struct qxc_stack_offsets* offsets,
                                    struct qxc_ast_block_item_node* block_item)
{
    if (block_item->type == STATEMENT_BLOCK_ITEM) {
        generate_statement_asm(gen, offsets, block_item->statement);
    }
    else if (block_item->type == DECLARATION_BLOCK_ITEM) {
        generate_declaration_asm(gen, offsets, block_item->declaration);
    }
    else {
        assert(block_item->type == INVALID_BLOCK_ITEM);
        debug_print("invalid block item encountered in code generation");
    }
}

// static void generate_function_asm(struct qxc_godegen* gen) {}

void generate_asm(struct qxc_program* program, const char* output_filepath)
{
    struct qxc_codegen gen;
    gen.indent_level = 0;

    gen.logical_or_counter = 0;
    gen.logical_and_counter = 0;
    gen.conditional_expr_counter = 0;

    gen.asm_output = fopen(output_filepath, "w");

    gen.indent_level++;
    emit(&gen, "global _start");
    emit(&gen, "section .text");
    gen.indent_level--;
    emit(&gen, "\n_start:");
    gen.indent_level++;

    emit(&gen, "push rbp");
    emit(&gen, "mov rbp, rsp");

    // TODO: we'll have to revisit this once we start to compile programs with
    // functions other than 'main'.
    struct qxc_stack_offsets offsets = qxc_stack_offsets_new();

    if (program->main_decl != NULL) {
        struct qxc_block_item_list* b = program->main_decl->blist;
        while (b != NULL && b->node != NULL) {
            generate_block_item_asm(&gen, &offsets, b->node);
            b = b->next_node;
        }
    }

    emit(&gen, "");
    emit(&gen, "mov rsp, rbp");
    emit(&gen, "pop rbp");
    emit(&gen, "ret");
    gen.indent_level--;

    fclose(gen.asm_output);

    qxc_memory_pool_release(program->ast_memory_pool);
}

