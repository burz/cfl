#include "cfl_compiler.h"

#include <iostream>
#include <vector>

cfl_Compiler::cfl_Compiler(void)
    : global_context(llvm::getGlobalContext())
{
}

llvm::Value* cfl_Compiler::compile_node_bool(cfl_node* node)
{
    bool value = *((bool*) node->data);

    return builder->getInt1(value);
}

llvm::Value* cfl_Compiler::compile_node_integer(cfl_node* node)
{
    int value = *((int*) node->data);

    return builder->getInt32(value);
}

llvm::Value* cfl_Compiler::compile_node_char(cfl_node* node)
{
    char value = *((char*) node->data);

    return builder->getInt8(value);
}

llvm::Value* cfl_Compiler::compile_node_and(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateAnd(left, right, "and");
}

llvm::Value* cfl_Compiler::compile_node_or(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateOr(left, right, "or");
}

llvm::Value* cfl_Compiler::compile_node_not(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* child = compile_node(node->children[0], parent);

    return builder->CreateNot(child, "not");
}

llvm::Value* cfl_Compiler::compile_node_add(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateAdd(left, right, "add");
}

llvm::Value* cfl_Compiler::compile_node_multiply(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateMul(left, right, "multiply");
}

llvm::Value* cfl_Compiler::compile_node_divide(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateSDiv(left, right, "divide");
}

llvm::Value* cfl_Compiler::compile_node_equal(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateICmpEQ(left, right, "equal");
}

llvm::Value* cfl_Compiler::compile_node_less(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateICmpSLT(left, right, "less");
}

llvm::Value* cfl_Compiler::compile_node_if(cfl_node* node, llvm::Function* parent)
{
    llvm::Value* condition = compile_node(node->children[0], parent);

    llvm::BasicBlock* if_true = llvm::BasicBlock::Create(
        global_context, "__if_true", parent);
    llvm::BasicBlock* if_false = llvm::BasicBlock::Create(
        global_context, "__if_false", parent);
    llvm::BasicBlock* if_end = llvm::BasicBlock::Create(
        global_context, "__if_end", parent);

    builder->CreateCondBr(condition, if_true, if_false);

    builder->SetInsertPoint(if_true);

    llvm::Value* then_value = compile_node(node->children[1], parent);

    builder->CreateBr(if_end);

    builder->SetInsertPoint(if_false);

    llvm::Value* else_value = compile_node(node->children[2], parent);

    builder->CreateBr(if_end);

    builder->SetInsertPoint(if_end);

    llvm::PHINode* phi = builder->CreatePHI(then_value->getType(), 2, "if_result");

    phi->addIncoming(then_value, if_true);
    phi->addIncoming(else_value, if_false);

    return phi;
}

llvm::Value* cfl_Compiler::compile_node(cfl_node* node, llvm::Function* parent)
{
    if(node->type == CFL_NODE_BOOL)
        return compile_node_bool(node);
    else if(node->type == CFL_NODE_INTEGER)
        return compile_node_integer(node);
    else if(node->type == CFL_NODE_CHAR)
        return compile_node_char(node);
    else if(node->type == CFL_NODE_AND)
        return compile_node_and(node, parent);
    else if(node->type == CFL_NODE_OR)
        return compile_node_or(node, parent);
    else if(node->type == CFL_NODE_NOT)
        return compile_node_not(node, parent);
    else if(node->type == CFL_NODE_ADD)
        return compile_node_add(node, parent);
    else if(node->type == CFL_NODE_MULTIPLY)
        return compile_node_multiply(node, parent);
    else if(node->type == CFL_NODE_DIVIDE)
        return compile_node_divide(node, parent);
    else if(node->type == CFL_NODE_EQUAL)
        return compile_node_equal(node, parent);
    else if(node->type == CFL_NODE_LESS)
        return compile_node_less(node, parent);
    else if(node->type == CFL_NODE_IF)
        return compile_node_if(node, parent);

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

void cfl_Compiler::generate_print_function(cfl_program* program)
{
    if(program->type->type == CFL_TYPE_BOOL)
    {
        std::vector<llvm::Type*> print_args;
        print_args.push_back(builder->getInt1Ty());
        llvm::ArrayRef<llvm::Type*> print_args_ref(print_args);

        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), print_args_ref, false);

        print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_bool", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_bool_entry", print_def);

        builder->SetInsertPoint(print_entry);

        llvm::BasicBlock* print_false = llvm::BasicBlock::Create(
            global_context, "__print_bool_false", print_def);
        llvm::BasicBlock* print_true = llvm::BasicBlock::Create(
            global_context, "__print_bool_true", print_def);
        llvm::BasicBlock* print_end = llvm::BasicBlock::Create(
            global_context, "__print_bool_end", print_def);

        llvm::Value* true_string = builder->CreateGlobalStringPtr("true");
        llvm::Value* false_string = builder->CreateGlobalStringPtr("false");

        builder->CreateCondBr(print_def->arg_begin()++, print_true, print_false);

        builder->SetInsertPoint(print_true);
        builder->CreateCall(global_puts, true_string);
        builder->CreateBr(print_end);

        builder->SetInsertPoint(print_false);
        builder->CreateCall(global_puts, false_string);
        builder->CreateBr(print_end);

        builder->SetInsertPoint(print_end);
        builder->CreateRetVoid();
    }
    else if(program->type->type == CFL_TYPE_INTEGER)
    {
        std::vector<llvm::Type*> print_args;
        print_args.push_back(builder->getInt32Ty());
        llvm::ArrayRef<llvm::Type*> print_args_ref(print_args);

        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), print_args_ref, false);

        print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_int", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_int_entry", print_def);

        builder->SetInsertPoint(print_entry);

        llvm::Value* format_string = builder->CreateGlobalStringPtr("%d\n");

        builder->CreateCall2(global_printf, format_string, print_def->arg_begin()++);
        builder->CreateRetVoid();
    }
    else if(program->type->type == CFL_TYPE_CHAR)
    {
        std::vector<llvm::Type*> print_args;
        print_args.push_back(builder->getInt8Ty());
        llvm::ArrayRef<llvm::Type*> print_args_ref(print_args);

        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), print_args_ref, false);

        print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_char", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_char_entry", print_def);

        builder->SetInsertPoint(print_entry);

        llvm::Value* format_string = builder->CreateGlobalStringPtr("'%c'\n");

        builder->CreateCall2(global_printf, format_string, print_def->arg_begin()++);
        builder->CreateRetVoid();
    }
    else
    {
        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), false);

        print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_generic", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_generic_entry", print_def);

        builder->SetInsertPoint(print_entry);

        llvm::Value* success_string = builder->CreateGlobalStringPtr("Success.");

        builder->CreateCall(global_puts, success_string);
        builder->CreateRetVoid();
    }
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

    generate_print_function(program);

    builder->SetInsertPoint(main_entry);

    llvm::Value* result = compile_node(program->main, main_def);

    if(!result)
        return false;

    builder->CreateCall(print_def, result);
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
