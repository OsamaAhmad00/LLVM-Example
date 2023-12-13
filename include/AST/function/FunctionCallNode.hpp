#pragma once

#include <AST/ASTNode.hpp>

namespace dua
{

class FunctionCallNode : public ASTNode
{
    friend class ParserAssistant;

protected:

    std::vector<Value> eval_args(bool is_method = false);

    std::string name;
    std::vector<ASTNode*> args;

public:

    FunctionCallNode(ModuleCompiler* compiler, std::string name, std::vector<ASTNode*> args = {})
            : name(std::move(name)), args(std::move(args)) { this->compiler = compiler; }

    Value eval() override;

    const Type* get_type() override;
};

}