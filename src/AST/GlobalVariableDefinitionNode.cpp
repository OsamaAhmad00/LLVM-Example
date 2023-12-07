#include <AST/variable/GlobalVariableDefinitionNode.hpp>
#include "AST/values/LLMVValueNode.hpp"
#include <utils/ErrorReporting.hpp>

namespace dua
{

llvm::GlobalVariable* GlobalVariableDefinitionNode::eval()
{
    // ASTNodes evaluation should be idempotent.
    // This condition makes sure this is the case.
    if (result != nullptr)
        return result;

    if (initializer != nullptr && !args.empty())
        report_error("Can't have both an initializer and an initializer list (in " + name + ")");

    module().getOrInsertGlobal(name, type->llvm_type());
    llvm::GlobalVariable* variable = module().getGlobalVariable(name);

    // We're in the global scope now, and the evaluation has to be done inside
    // some basic block. Will move temporarily to the beginning of the main function.
    auto old_position = builder().saveIP();
    builder().SetInsertPoint(&module().getFunction(".dua.init.")->getEntryBlock());

    llvm::Constant* constant = nullptr;

    if (initializer != nullptr)
    {
        auto value = compiler->create_value(initializer->eval(), initializer->get_type());

        auto llvm_value = typing_system().cast_value(value, type);
        if (llvm_value == nullptr)
            report_error("Type mismatch between the global variable " + name + " and its initializer");

        auto casted = llvm::dyn_cast<llvm::Constant>(llvm_value);
        // If it's not a constant, then the initialization happens in the
        // .dua.init function. Otherwise, initialized with the constant value.
        if (casted == nullptr) {
            builder().CreateStore(llvm_value, variable);
        } else {
            constant = casted;
        }
    }
    else if (!args.empty())
    {
        std::vector<Value> evaluated(args.size());
        for (int i = 0; i < args.size(); i++)
            evaluated[i] = compiler->create_value(args[i]->eval(), args[i]->get_type());

        name_resolver().call_constructor(compiler->create_value(variable, type), std::move(evaluated));
    }

    // Restore the old position back
    builder().restoreIP(old_position);

    variable->setInitializer(constant ? constant : type->default_value());
    variable->setConstant(false);

    name_resolver().symbol_table.insert_global(name, compiler->create_value(variable, type));

    return result = variable;
}

}