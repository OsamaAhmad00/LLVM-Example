#pragma once

#include <SymbolTable.hpp>
#include <resolution/FunctionNameResolver.hpp>
#include <types/FunctionType.hpp>
#include <types/ClassType.hpp>
#include <llvm/IR/IRBuilder.h>


namespace dua
{

class ASTNode;

struct FieldConstructorArgs
{
    std::string name;
    std::vector<ASTNode*> args;
};

class NameResolver
{

public:

    ModuleCompiler* compiler;

    SymbolTable<Value> symbol_table;
    std::unordered_map<std::string, const ClassType*> classes;
    // Instead of having the fields be stored in the class type,
    //  and having them getting duplicated on each clone, let's
    //  keep the fields info in one place, and refer to it by name.
    std::unordered_map<std::string, std::vector<ClassField>> class_fields;
    std::unordered_map<std::string, std::vector<FieldConstructorArgs>> fields_args;

    FunctionNameResolver function_resolver;

    explicit NameResolver(ModuleCompiler* compiler);

    [[nodiscard]] llvm::IRBuilder<>& builder() const;

    const ClassType* get_class(const std::string& name);
    void add_fields_constructor_args(std::string constructor_name, std::vector<FieldConstructorArgs> args);
    std::vector<FieldConstructorArgs>& get_fields_args(const std::string& constructor_name);

    void push_scope();
    Scope<Value> pop_scope();
    void destruct_all_variables(const Scope<Value>& scope);

    // Functions that delegate to the FunctionNameResolver
    void register_function(std::string name, FunctionInfo info, bool no_mangle = false);
    FunctionInfo& get_function(const std::string& name, const std::vector<const Type*>& param_types);
    FunctionInfo& get_function(const std::string& name, const std::vector<Value>& args);
    std::string get_function(const std::string& name);
    FunctionInfo& get_function_no_overloading(const std::string &name);
    [[nodiscard]] bool has_function(const std::string& name) const;
    llvm::CallInst* call_function(const std::string &name, std::vector<Value> args = {});
    llvm::CallInst* call_function(llvm::Value* ptr, const FunctionType* type, std::vector<Value> args = {});
    void call_constructor(const Value& value, std::vector<Value> args);
    void call_destructor(const Value& value);

    [[nodiscard]] static std::string get_full_function_name(std::string name, const std::vector<const Type*>& param_types);
};

}