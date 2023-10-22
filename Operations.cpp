#include "Compiler.h"

#define BINARY_OP(OP, EXP, LABEL, PARAM_RESTRICTION) \
assert(EXP.list.size() PARAM_RESTRICTION 3); \
llvm::Value* result = builder->OP(eval(EXP.list[1]), eval(EXP.list[2]), LABEL);\
for (int i = 3; i < EXP.list.size(); i++) \
    result = builder->OP(result, eval(EXP.list[i]));                    \
return result;

llvm::Value* Compiler::call_printf(const Expression& expression) {
    std::vector<llvm::Value*> args;
    for (int i = 1; i < expression.list.size(); i++) {
        args.push_back(eval(expression.list[i]));
    }
    return call_function("printf", args);
}

llvm::Value* Compiler::eval_scope(const Expression& expression) {
    symbol_table.push_scope();
    for (int i = 1; i < expression.list.size() - 1; i++)
        eval(expression.list[i]);
    llvm::Value* result = eval(expression.list.back());
    symbol_table.pop_scope();
    return result;
}

llvm::AllocaInst* Compiler::create_local_variable(const Expression& expression) {
    assert(expression.list.size() == 3);
    llvm::Constant* init = get_expression_value(expression.list[2]);
    return create_local_variable(expression.list[1].str, init);
}

llvm::GlobalVariable* Compiler::create_global_variable(const Expression& expression) {
    assert(expression.list.size() == 3);
    llvm::Constant* init = get_expression_value(expression.list[2]);
    return create_global_variable(expression.list[1].str, init);
}

llvm::Value* Compiler::set_variable(const Expression& expression) {
    assert(expression.list.size() == 3);
    auto& name = expression.list[1].str;
    auto& exp = expression.list[2];
    llvm::Value* result = symbol_table.contains(name) ?
            (llvm::Value*)symbol_table.get(name) : (llvm::Value*)symbol_table.get_global(name);
    builder->CreateStore(eval(exp), result);
    return result;
}

llvm::Value* Compiler::eval_sum(const Expression& expression) {
    BINARY_OP(CreateAdd, expression, "temp_add", >=)
}

llvm::Value* Compiler::eval_sub(const Expression& expression) {
    BINARY_OP(CreateSub, expression, "temp_sub", >=)
}

llvm::Value* Compiler::eval_mul(const Expression& expression) {
    BINARY_OP(CreateMul, expression, "temp_mul", >=)
}

llvm::Value* Compiler::eval_div(const Expression& expression) {
    BINARY_OP(CreateSDiv, expression, "temp_div", >=)
}

llvm::Value* Compiler::eval_less_than(const Expression& expression) {
    BINARY_OP(CreateICmpSLT, expression, "temp_lt", ==)
}

llvm::Value* Compiler::eval_greater_than(const Expression& expression) {
    BINARY_OP(CreateICmpSGT, expression, "temp_gt", ==)
}

llvm::Value* Compiler::eval_less_than_eq(const Expression& expression) {
    BINARY_OP(CreateICmpSLE, expression, "temp_lte", ==)
}

llvm::Value* Compiler::eval_greater_than_eq(const Expression& expression) {
    BINARY_OP(CreateICmpSGE, expression, "temp_gte", ==)
}

llvm::Value* Compiler::eval_equal(const Expression& expression) {
    BINARY_OP(CreateICmpEQ, expression, "temp_eq", ==)
}

llvm::Value* Compiler::eval_not_equal(const Expression& expression) {
    BINARY_OP(CreateICmpNE, expression, "temp_neq", ==)
}

llvm::Value* Compiler::eval_if(const Expression& expression) {
    assert(expression.list.size() == 4);
    const Expression& cond_exp = expression.list[1];
    const Expression& then_exp = expression.list[2];
    const Expression& else_exp = expression.list[3];

    // If a counter is not used, LLVM will assign numbers incrementally (for example then1, else2, condition3)
    //  which can be confusing, especially in nested expressions.
    static int _counter = 0;
    int counter = _counter++;  // storing a local copy
    // You might delay the attachment of the blocks to the functions to reorder the blocks in a more readable way
    //  by doing the following: current_function->getBasicBlockList().push_back(block); I prefer this way.
    llvm::BasicBlock* then_block = create_basic_block("then" + std::to_string(counter), current_function);
    llvm::BasicBlock* else_block = create_basic_block("else" + std::to_string(counter), current_function);
    llvm::BasicBlock* end_block = create_basic_block("if_end" + std::to_string(counter), current_function);


    llvm::Value* cond_res = eval(cond_exp);
    builder->CreateCondBr(cond_res, then_block, else_block);

    builder->SetInsertPoint(then_block);
    llvm::Value* then_res = eval(then_exp);
    // Since the block may contain nested statements, the current then_block may be terminated early, and after the eval
    //  of the then_exp, we end up in a completely different block. This block is where the branching instruction to
    //  the end_block will be inserted, and this block is the block that would be a predecessor of the end_block, instead
    //  of the current then_block. Thus, we need to use the block we ended up upon after evaluating the then_exp instead
    //  of the current then_block (the two blocks may be the same) when referring to the phi instruction, which expects
    //  the provided blocks to be its predecessor.
    builder->CreateBr(end_block);
    then_block = builder->GetInsertBlock();

    builder->SetInsertPoint(else_block);
    llvm::Value* else_res = eval(else_exp);
    // Same as the then block.
    builder->CreateBr(end_block);
    else_block = builder->GetInsertBlock();

    assert(then_res->getType() == else_res->getType());
    builder->SetInsertPoint(end_block);

    llvm::PHINode* phi = builder->CreatePHI(then_res->getType(), 2, "if_result" + std::to_string(counter));
    phi->addIncoming(then_res, then_block);
    phi->addIncoming(else_res, else_block);

    return phi;
}
