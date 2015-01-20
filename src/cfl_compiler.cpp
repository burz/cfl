#include "cfl_compiler.h"

cfl_Compiler::cfl_Compiler(void)
    : global_context(llvm::getGlobalContext())
{
}

llvm::Value* cfl_Compiler::compile_node_bool(cfl_node* node)
{
    llvm::Value* value;

    if(*((bool*) node->data))
        value = llvm::ConstantInt::getTrue(global_context);
    else
        value = llvm::ConstantInt::getFalse(global_context);

    return value;
}

std::string cfl_Compiler::compile_node_and(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node_bool(node->children[0]);
    llvm::Value* right = compile_node_bool(node->children[1]);

    std::string and_label = "and";

    llvm::Value* and_value = builder->CreateAdd(left, right, and_label);

    std::string and_result = "and_result";

    llvm::LoadInst* load = builder->CreateLoad(and_value, and_result);

    block->getInstList().push_back(load);

    return and_result;
}

std::string cfl_Compiler::compile_node(cfl_node* node, llvm::BasicBlock* block)
{
    return compile_node_and(node, block);
}

bool cfl_Compiler::compile_program(cfl_program* program, llvm::BasicBlock* main_entry)
{
    compile_node_and(program->main, main_entry);

    return true;
}

bool cfl_Compiler::compile(cfl_program* program, std::string& destination_file)
{
    builder = new llvm::IRBuilder<>(global_context);
    top_module = new llvm::Module("top", global_context);

    llvm::FunctionType* main_type = llvm::FunctionType::get(
        builder->getInt32Ty(), false);

    llvm::Function* main_def = llvm::Function::Create(
        main_type, llvm::Function::ExternalLinkage, "main", top_module);

    llvm::BasicBlock* main_entry = llvm::BasicBlock::Create(
        global_context, "main_entry", main_def);

    builder->SetInsertPoint(main_entry);

    cfl_error_division_by_zero_string =
        builder->CreateGlobalStringPtr("EVALUATION ERROR: Division by zero\n");

    if(!compile_program(program, main_entry))
    {
        delete builder;
        delete top_module;

        return false;
    }

    top_module->dump();

    delete builder;
    delete top_module;

    return true;
}
