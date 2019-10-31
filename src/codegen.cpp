#include "codegen.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "flat_map.h"

struct ScopeBlock {
};

#define QXC_STACK_OFFSETS_CAPACITY 64
struct StackOffsets {
    const char* variable_names[QXC_STACK_OFFSETS_CAPACITY];
    int variable_offsets[QXC_STACK_OFFSETS_CAPACITY];
    int stack_index;
    size_t count;

    StackOffsets* parent = nullptr;
};

static StackOffsets stack_offsets_new(void)
{
    StackOffsets offsets;
    offsets.count = 0;
    offsets.stack_index = -8;  // base of callers stack frame always saved as first entry
                               // of calee's stack frame, so start with -8 rather than 0
    return offsets;
}

static bool stack_offsets_contains(StackOffsets* offsets, const char* name)
{
    const size_t count = offsets->count;
    for (size_t i = 0; i < count; i++) {
        if (strs_are_equal(name, offsets->variable_names[i])) {
            return true;
        }
    }

    return false;
}

static int stack_offsets_insert(StackOffsets* offsets, const char* name)
{
    const size_t old_count = offsets->count;
    assert(old_count < QXC_STACK_OFFSETS_CAPACITY);
    offsets->variable_names[old_count] = name;
    offsets->variable_offsets[old_count] = offsets->stack_index;
    offsets->count++;
    offsets->stack_index -= 8;
    return 0;
}

static int stack_offsets_lookup(StackOffsets* offsets, const char* name)
{
    const size_t count = offsets->count;
    for (size_t i = 0; i < count; i++) {
        if (strs_are_equal(name, offsets->variable_names[i])) {
            return offsets->variable_offsets[i];
        }
    }
    exit(EXIT_FAILURE);
}

// static void stack_offsets_clear(StackOffsets* offsets)
// {
//     for (size_t i = 0; i < offsets->count; i++) {
//         offsets->variable_names[i] = nullptr;
//         offsets->variable_offsets[i] = 0;
//     }
//
//     offsets->count = 0;
// }

struct CodeGen {
    FILE* asm_output;
    size_t indent_level;

    size_t logical_or_counter;
    size_t logical_and_counter;
    size_t conditional_expr_counter;
};

#define JUMP_LABEL_MAX_LENGTH 64
struct JumpLabel {
    char buffer[JUMP_LABEL_MAX_LENGTH];
};

static void build_logical_or_jump_labels(CodeGen* gen, JumpLabel* snd_label,
                                         JumpLabel* end_label)
{
    size_t count = ++gen->logical_or_counter;
    qxc_snprintf(snd_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LOR_Snd_%zu", count);
    qxc_snprintf(end_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LOR_End_%zu", count);
}

static void build_logical_and_jump_labels(CodeGen* gen, JumpLabel* snd_label,
                                          JumpLabel* end_label)
{
    size_t count = ++gen->logical_and_counter;
    qxc_snprintf(snd_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LAND_Snd_%zu", count);
    qxc_snprintf(end_label->buffer, JUMP_LABEL_MAX_LENGTH, "_LAND_End_%zu", count);
}

static void build_conditional_expr_jump_labels(CodeGen* gen, JumpLabel* else_label,
                                               JumpLabel* post_label)
{
    size_t count = ++gen->conditional_expr_counter;
    qxc_snprintf(else_label->buffer, JUMP_LABEL_MAX_LENGTH, "_CondExpr_Else_%zu", count);
    qxc_snprintf(post_label->buffer, JUMP_LABEL_MAX_LENGTH, "_CondExpr_Post_%zu", count);
}

static void emit(CodeGen* gen, const char* fmt, ...)
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

static void generate_expression_asm(CodeGen* gen, StackOffsets* offsets,
                                    struct ExprNode* expr);

static void generate_logical_OR_binop_expression_asm(CodeGen* gen, StackOffsets* offsets,
                                                     BinopExpr* binop_node)
{
    assert(binop_node->op == Operator::LogicalOR);

    JumpLabel snd_label, end_label;
    build_logical_or_jump_labels(gen, &snd_label, &end_label);

    generate_expression_asm(gen, offsets, binop_node->left_expr);

    emit(gen, "cmp rax, 0");
    emit(gen, "je %s", snd_label.buffer);
    emit(gen, "mov rax, 1");
    emit(gen, "jmp %s", end_label.buffer);

    emit(gen, "\n%s:", snd_label.buffer);

    generate_expression_asm(gen, offsets, binop_node->right_expr);
    emit(gen, "cmp rax, 0");
    emit(gen, "mov rax, 0");
    emit(gen, "setne al");

    emit(gen, "\n%s:", end_label.buffer);
}

static void generate_logical_AND_binop_expression_asm(CodeGen* gen, StackOffsets* offsets,
                                                      BinopExpr* binop_node)
{
    assert(binop_node->op == Operator::LogicalAND);

    JumpLabel snd_label, end_label;
    build_logical_and_jump_labels(gen, &snd_label, &end_label);

    generate_expression_asm(gen, offsets, binop_node->left_expr);

    emit(gen, "cmp rax, 0");
    emit(gen, "jne %s", snd_label.buffer);
    emit(gen, "jmp %s", end_label.buffer);

    emit(gen, "%s:", snd_label.buffer);

    generate_expression_asm(gen, offsets, binop_node->right_expr);
    emit(gen, "cmp rax, 0");
    emit(gen, "mov rax, 0");
    emit(gen, "setne al");

    emit(gen, "%s:", end_label.buffer);
}

static void generate_assignment_binop_expression_asm(CodeGen* gen, StackOffsets* offsets,
                                                     BinopExpr* binop_node)
{
    assert(binop_node->left_expr && binop_node->right_expr);
    assert(binop_node->left_expr->type == ExprType::VariableRef);

    const char* varname = binop_node->left_expr->referenced_var_name;

    // generate value to be assigned to variable in left_expr
    generate_expression_asm(gen, offsets, binop_node->right_expr);

    if (!stack_offsets_contains(offsets, varname)) {
        fprintf(stderr, "attempted to assign value to un-initialized variable: %s\n",
                varname);
        exit(EXIT_FAILURE);
    }

    emit(gen, "mov [rbp + %d], rax", stack_offsets_lookup(offsets, varname));
}

static void generate_binop_expression_asm(CodeGen* gen, StackOffsets* offsets,
                                          BinopExpr* binop_node)
{
    const Operator op = binop_node->op;

    // separate treatment due to short circuiting
    if (op == Operator::LogicalOR) {
        generate_logical_OR_binop_expression_asm(gen, offsets, binop_node);
        return;
    }

    // separate treatment due to short circuiting
    if (op == Operator::LogicalAND) {
        generate_logical_AND_binop_expression_asm(gen, offsets, binop_node);
        return;
    }

    // separate treatment to avoid evaluating left expr
    if (op == Operator::Assignment) {
        generate_assignment_binop_expression_asm(gen, offsets, binop_node);
        return;
    }

    // Now, all remaining binary operations behave similarly.

    // Put left hand operand in rax, right hand operand in rbx
    generate_expression_asm(gen, offsets, binop_node->right_expr);
    emit(gen, "push rax");
    generate_expression_asm(gen, offsets, binop_node->left_expr);
    emit(gen, "pop rbx");

    switch (op) {
        case Operator::Plus:
            emit(gen, "add rax, rbx");
            break;
        case Operator::Minus:
            emit(gen, "sub rax, rbx");
            break;
        case Operator::Divide:
            emit(gen, "xor rdx, rdx");
            emit(gen, "idiv rbx");
            break;
        case Operator::Multiply:
            emit(gen, "imul rbx");
            break;
        case Operator::EqualTo:
            emit(gen, "cmp rax, rbx");
            emit(gen, "mov rax, 0");
            emit(gen, "sete al");
            break;
        case Operator::NotEqualTo:
            emit(gen, "cmp rax, rbx");
            emit(gen, "mov rax, 0");
            emit(gen, "setne al");
            break;
        case Operator::LessThan:
            emit(gen, "cmp rax, rbx");
            emit(gen, "mov rax, 0");
            emit(gen, "setl al");
            break;
        case Operator::LessThanOrEqualTo:
            emit(gen, "cmp rax, rbx");
            emit(gen, "mov rax, 0");
            emit(gen, "setle al");
            break;
        case Operator::GreaterThan:
            emit(gen, "cmp rax, rbx");
            emit(gen, "mov rax, 0");
            emit(gen, "setg al");
            break;
        case Operator::GreaterThanOrEqualTo:
            emit(gen, "cmp rax, rbx");
            emit(gen, "mov rax, 0");
            emit(gen, "setge al");
            break;
        default:
            QXC_UNREACHABLE();
            break;
    }
}

static void generate_expression_asm(CodeGen* gen, StackOffsets* offsets,
                                    struct ExprNode* node)
{
    switch (node->type) {
        case ExprType::IntLiteral:
            emit(gen, "mov rax, %d", node->literal);
            return;

        case ExprType::UnaryOp:
            generate_expression_asm(gen, offsets, node->unop_expr.child_expr);

            switch (node->unop_expr.op) {
                case Operator::Minus:
                    emit(gen, "neg rax");
                    break;
                case Operator::Complement:
                    emit(gen, "not rax");
                    break;
                case Operator::LogicalNegation:
                    emit(gen, "cmp rax, 0");
                    emit(gen, "mov rax, 0");
                    emit(gen, "sete al");
                    break;
                default:
                    QXC_UNREACHABLE();
                    break;
            }

            return;

        case ExprType::BinaryOp:
            generate_binop_expression_asm(gen, offsets, &node->binop_expr);
            return;

        case ExprType::Conditional: {
            JumpLabel else_label, post_label;
            build_conditional_expr_jump_labels(gen, &else_label, &post_label);

            generate_expression_asm(gen, offsets, node->cond_expr.conditional_expr);
            emit(gen, "cmp rax, 0");
            emit(gen, "je %s", else_label.buffer);
            generate_expression_asm(gen, offsets, node->cond_expr.if_expr);
            emit(gen, "jmp %s", post_label.buffer);
            emit(gen, "\n%s:", else_label.buffer);
            generate_expression_asm(gen, offsets, node->cond_expr.else_expr);
            emit(gen, "\n%s:", post_label.buffer);
            return;
        }

        case ExprType::VariableRef:
            if (!stack_offsets_contains(offsets, node->referenced_var_name)) {
                fprintf(stderr, "referenced unknown variable: %s\n",
                        node->referenced_var_name);
                exit(EXIT_FAILURE);
            }
            emit(gen, "mov rax, [rbp + %d]",
                 stack_offsets_lookup(offsets, node->referenced_var_name));
            return;

        default:
            QXC_UNREACHABLE();
            return;
    }
}

static void generate_statement_asm(CodeGen* gen, StackOffsets* offsets,
                                   StatementNode* statement_node)
{
    assert(statement_node);
    switch (statement_node->type) {
        case StatementType::Return:
            generate_expression_asm(gen, offsets, statement_node->return_expr);
            emit(gen, "mov rdi, rax");
            emit(gen, "mov rax, 60");  // syscall for exit
            emit(gen, "syscall");
            break;

        case StatementType::StandAloneExpr:
            generate_expression_asm(gen, offsets, statement_node->standalone_expr);
            break;

        case StatementType::IfElse: {
            JumpLabel else_label, post_label;
            build_conditional_expr_jump_labels(gen, &else_label, &post_label);

            IfElseStatement* ifelse_stmt = statement_node->ifelse_statement;

            // TODO: switch all nullptr to nullptr
            if (ifelse_stmt->else_branch_statement != nullptr) {
                generate_expression_asm(gen, offsets, ifelse_stmt->conditional_expr);
                emit(gen, "cmp rax, 0");
                emit(gen, "je %s", else_label.buffer);
                generate_statement_asm(gen, offsets, ifelse_stmt->if_branch_statement);
                emit(gen, "jmp %s", post_label.buffer);
                emit(gen, "\n%s:", else_label.buffer);
                generate_statement_asm(gen, offsets, ifelse_stmt->else_branch_statement);
                emit(gen, "\n%s:", post_label.buffer);
            }
            else {
                generate_expression_asm(gen, offsets, ifelse_stmt->conditional_expr);
                emit(gen, "cmp rax, 0");
                emit(gen, "je %s", post_label.buffer);
                generate_statement_asm(gen, offsets, ifelse_stmt->if_branch_statement);
                emit(gen, "\n%s:", post_label.buffer);
            }
            break;
        }

        default:
            QXC_UNREACHABLE();
            break;
    }
    return;
}

static void generate_declaration_asm(CodeGen* gen, StackOffsets* offsets,
                                     Declaration* declaration)
{
    if (stack_offsets_contains(offsets, declaration->var_name)) {
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
    stack_offsets_insert(offsets, declaration->var_name);
}

static void generate_block_item_asm(CodeGen* gen, StackOffsets* offsets,
                                    BlockItemNode* block_item)
{
    if (block_item->type == BlockItemType::Statement) {
        generate_statement_asm(gen, offsets, block_item->statement);
    }
    else if (block_item->type == BlockItemType::Declaration) {
        generate_declaration_asm(gen, offsets, block_item->declaration);
    }
    else {
        assert(block_item->type == BlockItemType::Invalid);
        debug_print("invalid block item encountered in code generation");
    }
}

// static void generate_function_asm(struct qxc_godegen* gen) {}

void generate_asm(Program* program, const char* output_filepath)
{
    CodeGen gen;
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
    StackOffsets offsets = stack_offsets_new();

    if (program->main_decl != nullptr) {
        for (BlockItemNode* b : program->main_decl->blist) {
            generate_block_item_asm(&gen, &offsets, b);
        }
    }

    emit(&gen, "");
    emit(&gen, "mov rsp, rbp");
    emit(&gen, "pop rbp");
    emit(&gen, "ret");
    gen.indent_level--;

    fclose(gen.asm_output);

    qxc_memory_pool_release(program->pool);
}

