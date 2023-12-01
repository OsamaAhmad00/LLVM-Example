#pragma once

#include <AST/values/ValueNode.h>
#include <types/ArrayType.h>

namespace dua
{

class ArrayValueNode : public ValueNode
{
    // An initialized array
    std::vector<llvm::Constant*> values;
    Type* element_type;
    size_t size;

public:

    ArrayValueNode(ModuleCompiler* compiler, size_t size, Type* element_type, std::vector<llvm::Constant*> values)
        : values(std::move(values)), size(size), element_type(element_type) { this->compiler = compiler; }

    llvm::Constant* eval() override {
        return llvm::ConstantArray::get((llvm::ArrayType*)(get_cached_type()->llvm_type()), values);
    }

    Type* compute_type() override {
        if (type == nullptr) return type = compiler->create_type<ArrayType>(element_type, size);
        return type;
    }

    ~ArrayValueNode() override { delete element_type; }
};

}
