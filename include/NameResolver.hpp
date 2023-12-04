#pragma once

#include <SymbolTable.hpp>
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

struct FunctionInfo
{
    FunctionType type;
    std::vector<std::string> param_names;
};

struct Variable
{
    llvm::Value* ptr;
    Type* type;
};

class NameResolver
{

public:

    ModuleCompiler* compiler;

    SymbolTable<Variable> symbol_table;
    std::unordered_map<std::string, FunctionInfo> functions;
    std::unordered_map<std::string, ClassType*> classes;
    // Instead of having the fields be stored in the class type,
    //  and having them getting duplicated on each clone, let's
    //  keep the fields info in one place, and refer to it by name.
    std::unordered_map<std::string, std::vector<ClassField>> class_fields;
    std::unordered_map<std::string, std::vector<FieldConstructorArgs>> fields_args;

    explicit NameResolver(ModuleCompiler* compiler);

    [[nodiscard]] llvm::IRBuilder<>& builder() const;

    ClassType* get_class(const std::string& name);
    void add_fields_constructor_args(std::string class_name, std::vector<FieldConstructorArgs> args);

    std::vector<FieldConstructorArgs>& get_fields_args(const std::string& class_name);
    void register_function(std::string name, FunctionInfo info);
    FunctionInfo& get_function(const std::string& name);

    [[nodiscard]] bool has_function(const std::string& name) const;
    void cast_function_args(std::vector<llvm::Value*>& args, const FunctionType& type);
    llvm::CallInst* call_function(const std::string &name, std::vector<llvm::Value*> args = {});
    llvm::CallInst* call_function(llvm::Value* ptr, const FunctionType& type, std::vector<llvm::Value*> args = {});
    void call_method_if_exists(const Variable& variable, const std::string& name, std::vector<llvm::Value*> args = {});

    void call_constructor(const Variable& variable, std::vector<llvm::Value*> args);
    void call_destructor(const Variable& variable);
    void destruct_all_variables(const Scope<Variable>& scope);

    void push_scope();
    Scope<Variable> pop_scope();

    ~NameResolver();
};

}