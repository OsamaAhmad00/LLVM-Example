#pragma once

#include <llvm/IR/Constant.h>
#include <llvm/IR/Type.h>

namespace dua
{

class ModuleCompiler;
class Value;
class ReferenceType;

struct Type
{
    ModuleCompiler* compiler = nullptr;
    virtual Value default_value() const = 0;
    virtual llvm::Type* llvm_type() const = 0;
    virtual std::string to_string() const = 0;
    virtual std::string as_key() const = 0;
    virtual bool operator==(const Type& other);
    virtual bool operator!=(const Type& other);
    const Type* get_contained_type() const;
    virtual ~Type() = default;

    llvm::Type* operator->() const;
    operator llvm::Type*() const;

    // Delegates to TypingSystem
    [[nodiscard]] const Type* get_winning_type(const Type* other, bool panic_on_failure=true, const std::string& message="") const;
    [[nodiscard]] bool is_castable(const Type* type) const;

    template <typename T>
    const T* as() const {
        auto res = dynamic_cast<const T*>(this);
        if (res == nullptr) {
            auto t = get_contained_type();
            if (t != nullptr) return t->as<T>();
        }
        return res;
    }

};

template <>
const ReferenceType* Type::as<ReferenceType>() const;

}