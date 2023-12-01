#pragma once

#include <AST/ASTNode.h>
#include <types/Type.h>

namespace dua
{

class CastExpressionNode : public ASTNode
{
    ASTNode* expression;
    Type* target_type;

public:

    CastExpressionNode(ModuleCompiler* compiler, ASTNode* expression, Type* target_type)
        : expression(expression), target_type(target_type) { this->compiler = compiler; }
    llvm::Value * eval() override;
    Type* compute_type() override;
    ~CastExpressionNode() override;
};

}
