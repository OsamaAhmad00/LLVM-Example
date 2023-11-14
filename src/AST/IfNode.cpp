#include <AST/IfNode.h>

int IfNode::_counter = 0;

llvm::PHINode* IfNode::eval()
{
    assert(!conditions.empty());

    int counter = _counter++;  // storing a local copy

    // Will be used later to insert a branch instruction at the end of it
    llvm::BasicBlock* old_block = builder().GetInsertBlock();

    std::vector<llvm::BasicBlock*> jump_to_blocks;
    std::vector<llvm::BasicBlock*> body_blocks;

    for (size_t i = 0; i < conditions.size(); i++)
    {
        llvm::BasicBlock* condition_block = create_basic_block(
                "if" + std::to_string(counter) + "_condition" + std::to_string(i),
                current_function()
        );

        llvm::BasicBlock* branch_block = create_basic_block(
                "if" + std::to_string(counter) + "_branch" + std::to_string(i),
                current_function()
        );

        jump_to_blocks.push_back(condition_block);
        body_blocks.push_back(branch_block);
    }

    llvm::BasicBlock* else_block = nullptr;

    if (has_else())
    {
         else_block = create_basic_block(
                "if" + std::to_string(counter) + "_branch_else",
                current_function()
        );

        jump_to_blocks.push_back(else_block);
    }

    builder().SetInsertPoint(old_block);
    builder().CreateBr(jump_to_blocks.front());

    llvm::BasicBlock* end_block  = create_basic_block(
            "if" + std::to_string(counter) + "_end",
            current_function()
    );
    jump_to_blocks.push_back(end_block);

    for (size_t i = 0; i < body_blocks.size(); i++)
    {
        builder().SetInsertPoint(jump_to_blocks[i]);
        llvm::Value* condition = conditions[i]->eval();
        condition = compiler->cast_value(condition, builder().getInt1Ty());
        if (condition == nullptr)
            throw std::runtime_error("The provided condition can't be casted to boolean value.");
        builder().CreateCondBr(condition, body_blocks[i], jump_to_blocks[i + 1]);
    }

    // The conditionals may be nested. And the block that
    //  branches to the block containing the phi node
    //  might not be the same as the block the code will
    //  be evaluated inside. For this, we need to keep
    //  track of where the block each branch has ended
    //  at after the evaluation.
    std::vector<llvm::BasicBlock*> phi_blocks;
    std::vector<llvm::Value*> values;

    // A quick hack to include the else block in the loop
    if (else_block) body_blocks.push_back(else_block);
    for (size_t i = 0; i < body_blocks.size(); i++) {
        builder().SetInsertPoint(body_blocks[i]);
        values.push_back(branches[i]->eval());
        builder().CreateBr(end_block);
        phi_blocks.push_back(builder().GetInsertBlock());
    }

    for (size_t i = 1; i < values.size(); i++) {
        values[i] = compiler->cast_value(values[i], values.front()->getType());
        if (values[i] == nullptr) {
            throw std::runtime_error("Mismatch in the types of the branches");
        }
    }

    builder().SetInsertPoint(end_block);
    llvm::PHINode* phi = builder().CreatePHI(
        values.front()->getType(),
        values.size(),
        "if" + std::to_string(counter) + "_result"
    );

    for (size_t i = 0; i < values.size(); i++) {
        phi->addIncoming(values[i], phi_blocks[i]);
    }

    return phi;
}


IfNode::~IfNode()
{
    for (auto ptr : conditions)
        delete ptr;
    for (auto ptr : branches)
        delete ptr;
}
