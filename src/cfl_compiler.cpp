#include "cfl_compiler.h"

#include <iostream>
#include <vector>

cfl_Compiler::cfl_Compiler(void)
    : global_context(llvm::getGlobalContext())
{
}

llvm::Value* cfl_Compiler::compile_node_bool(cfl_node* node)
{
    llvm::Constant* value;

    if(*((bool*) node->data))
        value = llvm::ConstantInt::getTrue(global_context);
    else
        value = llvm::ConstantInt::getFalse(global_context);

    return value;
}

llvm::Value* cfl_Compiler::compile_node_integer(cfl_node* node)
{
    int value = *((int*) node->data);

    llvm::Type* integer_type = builder->getInt32Ty();

    return llvm::ConstantInt::get(integer_type, value);
}

llvm::Value* cfl_Compiler::compile_node_char(cfl_node* node)
{
    char value = *((char*) node->data);

    llvm::Type* char_type = builder->getInt8Ty();

    return llvm::ConstantInt::get(char_type, value);
}

llvm::Value* cfl_Compiler::compile_node_and(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateAnd(left, right, "and");
}

llvm::Value* cfl_Compiler::compile_node_or(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateOr(left, right, "or");
}

llvm::Value* cfl_Compiler::compile_node_not(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* child = compile_node(node->children[0], block);

    return builder->CreateNot(child, "not");
}

llvm::Value* cfl_Compiler::compile_node_add(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateAdd(left, right, "add");
}

llvm::Value* cfl_Compiler::compile_node_multiply(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateMul(left, right, "multiply");
}

llvm::Value* cfl_Compiler::compile_node_divide(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateSDiv(left, right, "divide");
}

llvm::Value* cfl_Compiler::compile_node_equal(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateICmpEQ(left, right, "equal");
}

llvm::Value* cfl_Compiler::compile_node_less(cfl_node* node, llvm::BasicBlock* block)
{
    llvm::Value* left = compile_node(node->children[0], block);
    llvm::Value* right = compile_node(node->children[1], block);

    return builder->CreateICmpSLT(left, right, "less");
}

llvm::Value* cfl_Compiler::compile_node(cfl_node* node, llvm::BasicBlock* block)
{
    if(node->type == CFL_NODE_BOOL)
        return compile_node_bool(node);
    else if(node->type == CFL_NODE_INTEGER)
        return compile_node_integer(node);
    else if(node->type == CFL_NODE_CHAR)
        return compile_node_char(node);
    else if(node->type == CFL_NODE_AND)
        return compile_node_and(node, block);
    else if(node->type == CFL_NODE_OR)
        return compile_node_or(node, block);
    else if(node->type == CFL_NODE_NOT)
        return compile_node_not(node, block);
    else if(node->type == CFL_NODE_ADD)
        return compile_node_add(node, block);
    else if(node->type == CFL_NODE_MULTIPLY)
        return compile_node_multiply(node, block);
    else if(node->type == CFL_NODE_DIVIDE)
        return compile_node_divide(node, block);
    else if(node->type == CFL_NODE_EQUAL)
        return compile_node_equal(node, block);
    else if(node->type == CFL_NODE_LESS)
        return compile_node_less(node, block);

    return 0;
}

void cfl_Compiler::setup_global_defs(void)
{
    std::vector<llvm::Type*> puts_args;

    puts_args.push_back(builder->getInt8Ty()->getPointerTo());

    llvm::ArrayRef<llvm::Type*> puts_args_ref(puts_args);

    llvm::FunctionType* puts_type =
        llvm::FunctionType::get(builder->getInt32Ty(), puts_args_ref, false);

    global_puts = top_module->getOrInsertFunction("puts", puts_type);

    cfl_error_division_by_zero_string =
        builder->CreateGlobalStringPtr("EVALUATION ERROR: Division by zero");

    std::vector<llvm::Type*> printf_args;

    printf_args.push_back(builder->getInt8Ty()->getPointerTo());

    llvm::ArrayRef<llvm::Type*> printf_args_ref(printf_args);

    llvm::FunctionType* printf_type =
        llvm::FunctionType::get(builder->getInt32Ty(), printf_args_ref, true);

    global_printf = top_module->getOrInsertFunction("printf", printf_type);
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
