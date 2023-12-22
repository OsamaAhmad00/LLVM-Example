#pragma once

#include <AST/ASTNode.hpp>

namespace dua
{

class FunctionCallNode : public ASTNode
{
    friend class ParserAssistant;

protected:

    Value call_templated_function();

    std::vector<Value> eval_args(bool is_method = false);

    std::string name;
    std::vector<ASTNode*> args;
    std::vector<const Type*> template_args;
    bool is_templated;

public:

    FunctionCallNode(ModuleCompiler* compiler, std::string name,
                     std::vector<ASTNode*> args = {});
    FunctionCallNode(ModuleCompiler* compiler, std::string name,
                     std::vector<ASTNode*> args, std::vector<const Type*> template_args);

    Value eval() override;

    const Type* get_type() override;
};

}
