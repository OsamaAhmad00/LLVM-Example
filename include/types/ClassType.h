#pragma once

#include <types/TypeBase.h>
#include <llvm/IR/DerivedTypes.h>

namespace dua
{

struct ClassField
{
    std::string name;
    TypeBase* type;
    llvm::Constant* default_value;
};

class ClassType : public TypeBase
{

public:

    std::string name;
    // Methods are not stored in the class definition, rather, they're
    //  visible through the whole file, even before the class definition.
    //  In other words, class methods are treated just like regular functions.
    // We make use of the fact that the '.' character is not allowed to be in
    //  identifiers in the dua language, but allowed in LLVM IR. We prefix
    //  every function name with the name of the enclosing class, with a '.'
    //  in between (Printer.print for example). This way, we make sure that
    //  user defined function names can't collide with the method names.

    // The fields are stored and accessed through the compiler.

    ClassType(ModuleCompiler* compiler, std::string name, std::vector<ClassField> fields = {});

    llvm::Constant* default_value() override;

    llvm::StructType* llvm_type() const override;

    std::vector<ClassField>& fields();

    ClassType* clone() override;

    ~ClassType() override = default;
};

}