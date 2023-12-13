#pragma once

#include "LValueNode.hpp"

namespace dua
{

class VariableNode : public LValueNode
{

public:

    std::string name;

    VariableNode(ModuleCompiler* compiler, std::string name, const Type* type = nullptr);

    Value eval() override;

    const Type* get_type() override;

    const Type* get_element_type() override;

    [[nodiscard]] virtual bool is_function() const;
};

}