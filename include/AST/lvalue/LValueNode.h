#pragma once

#include "AST/ASTNode.h"

class LValueNode : public ASTNode
{
public:
    llvm::Value* eval() = 0;
};