#include <AST/terminals/VariableNode.h>

llvm::Value* VariableNode::eval()
{
    // This searches locally first, then globally if not found.
    auto variable = symbol_table().get(name);

    if (get_address)
        return variable.ptr;

    return builder().CreateLoad(variable.type, variable.ptr);
}