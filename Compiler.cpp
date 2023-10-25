#include "Compiler.h"
#include "Parser.h"

Compiler::Compiler(const std::string& name) :
    context(std::make_unique<llvm::LLVMContext>()),
    module(std::make_unique<llvm::Module>(name, *context)),
    builder(std::make_unique<llvm::IRBuilder<>>(*context)),
    temp_builder(std::make_unique<llvm::IRBuilder<>>(*context)),
    parser(new syntax::Parser)
{
    init_external_references();
    init_primitive_types();
}

Compiler::~Compiler() { delete parser; }

void Compiler::compile(const std::string& code, const std::string& outfile) {

    // Parse
    Expression expression = parser->parse(code);

    // Generate IR
    eval_main(expression);

    // Output
    module->print(llvm::outs(), nullptr);  // Print
    save_module(outfile);
}

void Compiler::init_primitive_types() {
    types["int"] = builder->getInt32Ty();
    types["str"] = builder->getInt8PtrTy();
}

llvm::GlobalVariable* Compiler::create_global_variable(const std::string& name, llvm::Type* type, const Expression& init_exp)
{
    module->getOrInsertGlobal(name, type);
    llvm::GlobalVariable* variable = module->getGlobalVariable(name);
    variable->setConstant(false);
    variable->setAlignment(llvm::MaybeAlign(4));
    variable->setExternallyInitialized(false);
    if (llvm::Constant *value = get_constant(init_exp); value)
        variable->setInitializer(value);
    else
        throw std::runtime_error("Initialization of a global variable with non-constant value");
    symbol_table.insert_global(name, variable);
    return variable;
}

llvm::AllocaInst* Compiler::create_local_variable(const std::string& name, llvm::Type* type, const Expression* init_exp)
{
    llvm::BasicBlock* entry = &current_function->getEntryBlock();
    temp_builder->SetInsertPoint(entry, entry->begin());
    llvm::AllocaInst* variable = temp_builder->CreateAlloca(type, 0, name);
    if (init_exp) {
        llvm::Value* initializer = get_constant(*init_exp);
        if (!initializer) initializer = eval(*init_exp);
        builder->CreateStore(initializer, variable);
    }
    symbol_table.insert(name, variable);
    return variable;
}

llvm::LoadInst* Compiler::get_local_variable(const std::string& name)
{
    llvm::AllocaInst* variable = symbol_table.get(name);
    return builder->CreateLoad(variable->getAllocatedType(), variable);
}

llvm::LoadInst* Compiler::get_global_variable(const std::string& name)
{
    llvm::GlobalVariable* variable = symbol_table.get_global(name);
    return builder->CreateLoad(variable->getValueType(), variable);
}

llvm::Value* Compiler::eval(const Expression& expression) {
    switch (expression.type)
    {
        case SExpressionType::SYMBOL:
            if (expression.str == "true" || expression.str == "false") {
                return builder->getInt1(expression.str == "true");
            } else {
                if (symbol_table.contains_global(expression.str))
                    return get_global_variable(expression.str);
                return get_local_variable(expression.str);
            }
        case SExpressionType::STRING:
            static int str_lit_cnt = 0;
            return create_string_literal("literal_" + std::to_string(str_lit_cnt++), expression.str);
        case SExpressionType::NUMBER:
            return create_integer_literal(expression.num);
        case SExpressionType::LIST:
            const Expression& first = expression.list.front();
            if (first.type == SExpressionType::SYMBOL) {
                if (first.str == "printf")
                    return call_printf(expression);
                else if (first.str == "scope")
                    return eval_scope(expression);
                else if (first.str == "global")
                    return create_global_variable(expression);
                else if (get_type(first.str) != nullptr)
                    return create_local_variable(expression);
                else if (first.str == "set")
                    return set_variable(expression);
                else if (first.str == "+")
                    return eval_sum(expression);
                else if (first.str == "-")
                    return eval_sub(expression);
                else if (first.str == "*")
                    return eval_mul(expression);
                else if (first.str == "/")
                    return eval_div(expression);
                else if (first.str == "<")
                    return eval_less_than(expression);
                else if (first.str == ">")
                    return eval_greater_than(expression);
                else if (first.str == "<=")
                    return eval_less_than_eq(expression);
                else if (first.str == ">=")
                    return eval_greater_than_eq(expression);
                else if (first.str == "==")
                    return eval_equal(expression);
                else if (first.str == "!=")
                    return eval_not_equal(expression);
                else if (first.str == "if")
                    return eval_if(expression);
                else if (first.str == "while")
                    return eval_while(expression);
                else
                    throw std::runtime_error("Not supported operation");
            }
            return eval(expression.list.back());
    }
    return nullptr;
}

llvm::Constant* Compiler::get_default_value(llvm::Type* type) {
    auto result = default_values.find(type);
    assert(result != default_values.end());
    return result->second;
}

llvm::Type* Compiler::get_type(const std::string& str) {
    auto result = types.find(str);
    if (result == types.end()) return nullptr;
    return result->second;
}

llvm::Constant* Compiler::get_constant(const Expression& expression) {
    switch (expression.type) {
        case SExpressionType::NUMBER:
            return builder->getInt32(expression.num);
        case SExpressionType::STRING:
            return builder->CreateGlobalStringPtr(expression.str);
        default:  // Symbol or List
            return nullptr;
    }
}

llvm::ConstantInt* Compiler::create_integer_literal(long long num) {
    return builder->getInt32(num);
}

llvm::Constant* Compiler::create_string_literal(const std::string& name, const std::string& str) {
    return builder->CreateGlobalStringPtr(str, name);
}

llvm::CallInst* Compiler::call_function(const std::string& name, const std::vector<llvm::Value*>& args) {
    llvm::Function* function = module->getFunction(name);
    return builder->CreateCall(function, args);
}

void Compiler::eval_main(Expression& expression) {
    llvm::Function* function = create_function("main");
    construct_function_body(function, expression);
}

void Compiler::construct_function_body(llvm::Function* function, Expression& body) {

    llvm::IRBuilderBase::InsertPoint old_insert_point = builder->saveIP();
    builder->SetInsertPoint(&function->back());

    eval(body);

    builder->CreateRet(builder->getInt32(0));

    builder->restoreIP(old_insert_point);
}

llvm::Function* Compiler::create_function(const std::string& name) {
    llvm::Function* function = module->getFunction(name);

    if (!function) {
        llvm::FunctionType* type = llvm::FunctionType::get(builder->getInt32Ty(), false);
        function = create_function_prototype(name, type);
    }

    create_basic_block("entry", function);

    return function;
}

llvm::Function* Compiler::create_function_prototype(const std::string& name, llvm::FunctionType* type) {
    llvm::Function* function = llvm::Function::Create(type, llvm::Function::ExternalLinkage, name, *module);
    llvm::verifyFunction(*function);
    current_function = function;
    return function;
}

llvm::BasicBlock* Compiler::create_basic_block(const std::string& name, llvm::Function* function) {
    return llvm::BasicBlock::Create(*context, name, function);
}

void Compiler::save_module(const std::string& outfile) {
    std::error_code error;
    llvm::raw_fd_ostream out(outfile, error);
    module->print(out, nullptr);
}

void Compiler::init_external_references() {
    // i32 printf(i8*, ...)
    llvm::FunctionType* type = llvm::FunctionType::get(builder->getInt32Ty(), {builder->getInt8PtrTy()}, true);
    module->getOrInsertFunction("printf", type);
}