#pragma once

#include "allocator.h"
#include "array.h"
#include "prelude.h"
#include "token.h"

// --------------------------------------------------------------------------------

enum class ExprType { IntLiteral, UnaryOp, BinaryOp, VariableRef, Conditional, Invalid };

struct UnopExpr {
    Operator op = Operator::Invalid;
    struct ExprNode* child_expr = nullptr;
};

struct BinopExpr {
    Operator op = Operator::Invalid;
    struct ExprNode* left_expr = nullptr;
    struct ExprNode* right_expr = nullptr;
};

struct CondExpr {
    struct ExprNode* conditional_expr = nullptr;
    struct ExprNode* if_expr = nullptr;
    struct ExprNode* else_expr = nullptr;
};

struct ExprNode {
    ExprType type = ExprType::Invalid;

    union {
        long literal;
        struct UnopExpr unop_expr;
        struct BinopExpr binop_expr;
        struct CondExpr cond_expr;
        const char* referenced_var_name;
    };
};

// --------------------------------------------------------------------------------

enum class StatementType { Return, StandAloneExpr, IfElse, Compound, Invalid };

struct StatementNode;

struct IfElseStatement {
    ExprNode* conditional_expr = nullptr;
    StatementNode* if_branch_statement = nullptr;
    StatementNode* else_branch_statement = nullptr;  // optional, may be NULL
};

struct BlockItemNode;

struct StatementNode {
    StatementType type = StatementType::Invalid;

    union {
        ExprNode* return_expr;
        IfElseStatement* ifelse_statement;
        array<BlockItemNode*> compound_statement_block_items;
        ExprNode* standalone_expr;
    };
};

// --------------------------------------------------------------------------------

struct Declaration {
    char* var_name = nullptr;
    struct ExprNode* initializer_expr = nullptr;
};

// --------------------------------------------------------------------------------

enum class BlockItemType { Statement, Declaration, Invalid };

struct BlockItemNode {
    BlockItemType type = BlockItemType::Invalid;

    union {
        StatementNode* statement;
        Declaration* declaration;
    };
};

// --------------------------------------------------------------------------------

struct FunctionDecl {
    const char* name = nullptr;
    array<BlockItemNode*> blist;
};

// --------------------------------------------------------------------------------

struct Program {  // program
    FunctionDecl* main_decl = nullptr;
    struct qxc_memory_pool* pool = nullptr;
};

Program* qxc_parse(const char* filepath);

