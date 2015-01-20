#include "cfl_compiler.h"

#include <iostream>
#include <vector>

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

llvm::Value* cfl_Compiler::compile_node_and(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateAnd(left, right, "and_value");
}

llvm::Value* cfl_Compiler::compile_node(cfl_node* node, llvm::BasicBlock* block)
{
    if(node->type == CFL_NODE_BOOL)
        return compile_node_bool(node);
    else if(node->type == CFL_NODE_AND)
        return compile_node_and(node, block);

    return 0;
}

void cfl_Compiler::setup_global_defs(void)
{
    std::vector<llvm::Type*> puts_args;

    puts_args.push_back(builder->getInt8Ty()->getPointerTo());

    llvm::ArrayRef<llvm::Type*> args_ref(puts_args);

    llvm::FunctionType *puts_type =
        llvm::FunctionType::get(builder->getInt32Ty(), args_ref, false);

    global_puts = top_module->getOrInsertFunction("puts", puts_type);

    cfl_error_division_by_zero_string =
        builder->CreateGlobalStringPtr("EVALUATION ERROR: Division by zero");
}

bool cfl_Compiler::compile_program(cfl_program* program)
{
    llvm::FunctionType* main_type = llvm::FunctionType::get(
        builder->getInt32Ty(), false);

    llvm::Function* main_def = llvm::Function::Create(
        main_type, llvm::Function::ExternalLinkage, "main", top_module);

    llvm::BasicBlock* main_entry = llvm::BasicBlock::Create(
        global_context, "main_entry", main_def);

    builder->SetInsertPoint(main_entry);

    setup_global_defs();

    llvm::Value* result = compile_node(program->main, main_entry);

    if(!result)
        return false;

    builder->CreateCall(global_puts, cfl_error_division_by_zero_string);
    builder->CreateRet(llvm::ConstantInt::get(builder->getInt32Ty(), 0));

    return true;
}

bool cfl_Compiler::compile(cfl_program* program, std::string& destination_file)
{
    builder = new llvm::IRBuilder<>(global_context);
    top_module = new llvm::Module("top", global_context);

    if(!compile_program(program))
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
