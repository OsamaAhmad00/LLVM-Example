#include <ModuleCompiler.h>
#include "parsing/ParserFacade.h"
#include <AST/TranslationUnitNode.h>
#include <llvm/Support/Host.h>
#include <utils/ErrorReporting.h>
#include <llvm/IR/Verifier.h>
#include <types/PointerType.h>

namespace dua
{

ModuleCompiler::ModuleCompiler(const std::string &module_name, const std::string &code) :
    context(),
    module(module_name, context),
    builder(context),
    temp_builder(context)
{
    module.setTargetTriple(llvm::sys::getDefaultTargetTriple());

    ParserFacade parser(*this);

    // Parse
    TranslationUnitNode* ast = parser.parse(code);

    // Generate LLVM IR
    ast->eval();

    auto& insertion_point = module.getFunction("main")->front().front();
    builder.SetInsertPoint(&insertion_point);
    for (auto node : deferred_nodes)
        node->eval();

    llvm::raw_string_ostream stream(result);
    module.print(stream, nullptr);
    result = stream.str();

    delete ast;
}

llvm::Value* ModuleCompiler::cast_value(llvm::Value* value, llvm::Type* target_type, bool panic_on_failure)
{
    llvm::Type* source_type = value->getType();

    // If the types are equal, return the value as it is
    if (source_type == target_type) {
        return value;
    }

    llvm::DataLayout dl(&module);
    unsigned int source_width = dl.getTypeAllocSize(source_type);
    unsigned int target_width = dl.getTypeAllocSize(target_type);

    // If the types are both integer types, use the Trunc or ZExt or SExt instructions
    if (source_type->isIntegerTy() && target_type->isIntegerTy())
    {
        if (source_width >= target_width) {
            // This needs to be >=, not just > because of the case of
            //  converting between an i8 and i1, which both give a size
            //  of 1 byte.
            // Truncate the value to fit the smaller type
            return builder.CreateTrunc(value, target_type);
        } else if (source_width < target_width) {
            // Extend the value to fit the larger type
            return builder.CreateSExt(value, target_type);
        }
    }

    if (source_type->isIntegerTy() && target_type->isPointerTy())
        return builder.CreateIntToPtr(value, target_type);

    if (source_type->isFloatingPointTy() && target_type->isFloatingPointTy()) {
        if (source_width > target_width) {
            // Truncate the value to fit the smaller type
            return builder.CreateFPTrunc(value, target_type);
        } else if (source_width < target_width) {
            // Extend the value to fit the larger type
            return builder.CreateFPExt(value, target_type);
        } else {
            // The types have the same bit width, no need to cast
            return value;
        }
    }

    if (source_type->isIntegerTy() && target_type->isFloatingPointTy())
        return builder.CreateSIToFP(value, target_type);

    if (source_type->isFloatingPointTy() && target_type->isIntegerTy())
        return builder.CreateFPToSI(value, target_type);

    source_type->print(llvm::outs());
    llvm::outs() << '\n';
    target_type->print(llvm::outs());
    llvm::outs() << '\n';

    if (panic_on_failure)
        report_error("Casting couldn't be done");

    return nullptr;
}

TypeBase* ModuleCompiler::get_winning_type(TypeBase* lhs, TypeBase* rhs)
{
    auto l = lhs->llvm_type();
    auto r = rhs->llvm_type();

    if (l == r) {
        return lhs;
    }

    llvm::DataLayout dl(&module);
    unsigned int l_width = dl.getTypeAllocSize(l);
    unsigned int r_width = dl.getTypeAllocSize(r);

    if (l->isIntegerTy() && r->isIntegerTy())
        return (l_width >= r_width) ? lhs : rhs;

    if (l->isPointerTy() && r->isIntegerTy())
        return lhs;

    if (l->isIntegerTy() && r->isPointerTy())
        return rhs;

    if (l->isFloatingPointTy() && r->isFloatingPointTy())
        return (l_width >= r_width) ? lhs : rhs;

    if (l->isIntegerTy() && r->isFloatingPointTy())
        return rhs;

    if (l->isFloatingPointTy() && r->isIntegerTy())
        return rhs;

    report_internal_error("Type mismatch");
    return nullptr;
}

void ModuleCompiler::register_function(std::string name, FunctionInfo info)
{
    if (current_function != nullptr)
        report_internal_error("Nested functions are not allowed");

    // Registering the function in the module so that all
    //  functions are visible during AST evaluation.
    llvm::Type* ret = info.type.return_type->llvm_type();
    std::vector<llvm::Type*> parameter_types;
    for (auto& type: info.type.param_types)
        parameter_types.push_back(type->llvm_type());
    llvm::FunctionType* type = llvm::FunctionType::get(ret, parameter_types, info.type.is_var_arg);
    llvm::Function* function = llvm::Function::Create(type, llvm::Function::ExternalLinkage, name, module);
    llvm::verifyFunction(*function);

    auto it = functions.find(name);
    if (it != functions.end()) {
        // Verify that the signatures are the same
        if (it->second.type != info.type) {
            report_error("Redefinition of the function " + name + " with a different signature");
        }
    } else {
        functions[std::move(name)] = std::move(info);
    }
}

FunctionInfo& ModuleCompiler::get_function(const std::string& name)
{
    auto it = functions.find(name);

    if (it != functions.end())
        return it->second;

    size_t dot = 0;
    while (dot < name.size() && name[dot] != '.') dot++;

    if (dot == name.size()) {
        report_error("The function " + name + " is not declared/defined");
    } else {
        // A class method
        auto class_name = name.substr(0, dot);
        auto method_name = name.substr(dot + 1);
        report_error("The class " + class_name + " is not defined. Can't resolve " + class_name + "::" + method_name);
    }

    // Unreachable
    return it->second;
}

llvm::Value *ModuleCompiler::cast_as_bool(llvm::Value *value, bool panic_on_failure) {
    value = cast_value(value, builder.getInt64Ty(), panic_on_failure);
    return builder.CreateICmpNE(value, builder.getInt64(0));
}

void ModuleCompiler::call_method_if_exists(const Variable& variable, const std::string& name)
{
    if (auto class_type = dynamic_cast<ClassType *>(variable.type); class_type != nullptr) {
        std::string constructor = class_type->name + "." + name;
        if (functions.find(constructor) != functions.end()) {
            auto func = module.getFunction(constructor);
            builder.CreateCall(func, variable.ptr);
        }
    }
}

void ModuleCompiler::destruct_all_variables(const Scope<Variable> &scope)
{
    for (auto& variable : scope.map)
        call_method_if_exists(variable.second, "destructor");
}

bool ModuleCompiler::has_function(const std::string &name) const {
    return functions.find(name) != functions.end();
}


void ModuleCompiler::define_function_alias(const std::string &from, const std::string &to) {
    function_aliases.insert(from, to);
}

llvm::CallInst* ModuleCompiler::call_function(const std::string &name, std::vector<llvm::Value*> args)
{
    llvm::Function* function = module.getFunction(name);
    auto& info = get_function(name);

    for (size_t i = args.size() - 1; i != (size_t)-1; i--) {
        if (i < info.param_names.size()) {
            // Only try to cast non-var-arg parameters
            auto param_type = info.type.param_types[i]->llvm_type();
            args[i] = cast_value(args[i], param_type);
        } else if (args[i]->getType() == builder.getFloatTy()) {
            // Variadic functions promote floats to doubles
            args[i] = cast_value(args[i], builder.getDoubleTy());
        }
    }

    return builder.CreateCall(function, std::move(args));
}

void ModuleCompiler::push_scope() {
    symbol_table.push_scope();
    function_aliases.push_scope();
}

Scope<Variable> ModuleCompiler::pop_scope() {
    function_aliases.pop_scope();
    return symbol_table.pop_scope();
}

}
