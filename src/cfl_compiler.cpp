#include "cfl_compiler.h"

#include <iostream>
#include <vector>

cfl_Compiler::cfl_Compiler(void)
    : global_context(llvm::getGlobalContext())
{
}

llvm::Type* cfl_Compiler::generate_type(cfl_type* type)
{
    if(type->type == CFL_TYPE_BOOL)
        return builder->getInt1Ty();
    else if(type->type == CFL_TYPE_INTEGER)
        return builder->getInt32Ty();
    else if(type->type == CFL_TYPE_CHAR)
        return builder->getInt8Ty();
    else if(type->type == CFL_TYPE_LIST)
        return builder->getInt8PtrTy();
    else if(type->type == CFL_TYPE_TUPLE)
        return llvm::ArrayType::get(builder->getInt8PtrTy(), type->id);
    else if(type->type == CFL_TYPE_ARROW)
        return builder->getInt8PtrTy();

    return 0;
}

llvm::Value* cfl_Compiler::compile_node_bool(cfl_typed_node* node)
{
    bool value = *((bool*) node->data);

    return builder->getInt1(value);
}

llvm::Value* cfl_Compiler::compile_node_integer(cfl_typed_node* node)
{
    int value = *((int*) node->data);

    return builder->getInt32(value);
}

llvm::Value* cfl_Compiler::compile_node_char(cfl_typed_node* node)
{
    char value = *((char*) node->data);

    return builder->getInt8(value);
}

llvm::Value* cfl_Compiler::compile_node_list(cfl_typed_node* node, llvm::Function* parent)
{
    return 0;
}

llvm::Value* cfl_Compiler::compile_node_tuple(cfl_typed_node* node, llvm::Function* parent)
{
    std::vector<llvm::Value*> tuple;
    std::vector<llvm::Constant*> zeroes;

    int i = 0;
    for( ; i < node->number_of_children; ++i)
    {
        llvm::Value* child = compile_node(node->children[i], parent);

        if(!child)
            return 0;

        llvm::Type* child_type =
            generate_type(((cfl_type**) node->resulting_type->input)[i]);

        if(!child_type)
            return 0;

        llvm::Constant* size = builder->getInt32(1);

        llvm::AllocaInst* child_space =
            builder->CreateAlloca(child_type, size, "child_allocate");

        builder->CreateStore(child, child_space, "child_store");

        llvm::Value* child_pointer = builder->CreatePointerCast(
            child_space, builder->getInt8PtrTy(), "child_cast");

        tuple.push_back(child_pointer);
        zeroes.push_back(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    }

    llvm::ArrayType* array_type =
        llvm::ArrayType::get(builder->getInt8PtrTy(), tuple.size());

    llvm::ArrayRef<llvm::Constant*> array_ref(zeroes);

    llvm::Value* array = llvm::ConstantArray::get(array_type, array_ref);

    std::vector<llvm::Value*>::iterator itt = tuple.begin();
    std::vector<llvm::Value*>::iterator end = tuple.end();

    for(i = 0; itt != end; ++itt)
    {
        array = builder->CreateInsertValue(array, *itt, i);

        ++i;
    }

    return array;
}

llvm::Value* cfl_Compiler::compile_node_and(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateAnd(left, right, "and");
}

llvm::Value* cfl_Compiler::compile_node_or(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateOr(left, right, "or");
}

llvm::Value* cfl_Compiler::compile_node_not(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* child = compile_node(node->children[0], parent);

    return builder->CreateNot(child, "not");
}

llvm::Value* cfl_Compiler::compile_node_add(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateAdd(left, right, "add");
}

llvm::Value* cfl_Compiler::compile_node_multiply(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateMul(left, right, "multiply");
}

llvm::Value* cfl_Compiler::compile_node_divide(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateSDiv(left, right, "divide");
}

llvm::Value* cfl_Compiler::compile_node_equal(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateICmpEQ(left, right, "equal");
}

llvm::Value* cfl_Compiler::compile_node_less(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateICmpSLT(left, right, "less");
}

llvm::Value* cfl_Compiler::compile_node_if(cfl_typed_node* node, llvm::Function* parent)
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

llvm::Value* cfl_Compiler::compile_node(cfl_typed_node* node, llvm::Function* parent)
{
    if(node->node_type == CFL_NODE_BOOL)
        return compile_node_bool(node);
    else if(node->node_type == CFL_NODE_INTEGER)
        return compile_node_integer(node);
    else if(node->node_type == CFL_NODE_CHAR)
        return compile_node_char(node);
    else if(node->node_type == CFL_NODE_LIST)
        return compile_node_list(node, parent);
    else if(node->node_type == CFL_NODE_TUPLE)
        return compile_node_tuple(node, parent);
    else if(node->node_type == CFL_NODE_AND)
        return compile_node_and(node, parent);
    else if(node->node_type == CFL_NODE_OR)
        return compile_node_or(node, parent);
    else if(node->node_type == CFL_NODE_NOT)
        return compile_node_not(node, parent);
    else if(node->node_type == CFL_NODE_ADD)
        return compile_node_add(node, parent);
    else if(node->node_type == CFL_NODE_MULTIPLY)
        return compile_node_multiply(node, parent);
    else if(node->node_type == CFL_NODE_DIVIDE)
        return compile_node_divide(node, parent);
    else if(node->node_type == CFL_NODE_EQUAL)
        return compile_node_equal(node, parent);
    else if(node->node_type == CFL_NODE_LESS)
        return compile_node_less(node, parent);
    else if(node->node_type == CFL_NODE_IF)
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

llvm::Value* cfl_Compiler::extract_value_from_pointer(
        llvm::Value* pointer,
        cfl_type* type)
{
    if(type->type == CFL_TYPE_BOOL)
    {
        llvm::PointerType* bool_pointer_type =
            llvm::PointerType::getUnqual(builder->getInt1Ty());

        llvm::Value* bool_pointer =
            builder->CreatePointerCast(pointer, bool_pointer_type);

        return builder->CreateLoad(bool_pointer, "load_bool");
    }
    else if(type->type == CFL_TYPE_INTEGER)
    {
        llvm::PointerType* integer_pointer_type =
            llvm::PointerType::getUnqual(builder->getInt32Ty());

        llvm::Value* integer_pointer =
            builder->CreatePointerCast(pointer, integer_pointer_type);

        return builder->CreateLoad(integer_pointer, "load_integer");
    }
    else if(type->type == CFL_TYPE_CHAR)
        return builder->CreateLoad(pointer, "load_char");
    else if(type->type == CFL_TYPE_TUPLE)
    {
        llvm::ArrayType* tuple_type =
            llvm::ArrayType::get(builder->getInt8PtrTy(), type->id);

        llvm::PointerType* tuple_pointer_type =
            llvm::PointerType::getUnqual(tuple_type);

        llvm::Value* tuple_pointer =
            builder->CreatePointerCast(pointer, tuple_pointer_type);

        return builder->CreateLoad(tuple_pointer, "load_tuple");
    }

    return 0;
}

void cfl_Compiler::generate_print_function(
        cfl_type* result_type,
        llvm::Value* result,
        llvm::BasicBlock* block)
{
    if(result_type->type == CFL_TYPE_BOOL)
    {
        llvm::Function* lookup = top_module->getFunction("__print_bool");

        if(lookup)
        {
            builder->CreateCall(lookup, result);

            return;
        }

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
        builder->CreateCall(global_printf, true_string);
        builder->CreateBr(print_end);

        builder->SetInsertPoint(print_false);
        builder->CreateCall(global_printf, false_string);
        builder->CreateBr(print_end);

        builder->SetInsertPoint(print_end);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def, result);
    }
    else if(result_type->type == CFL_TYPE_INTEGER)
    {
        llvm::Function* lookup = top_module->getFunction("__print_integer");

        if(lookup)
        {
            builder->CreateCall(lookup, result);

            return;
        }

        std::vector<llvm::Type*> print_args;
        print_args.push_back(builder->getInt32Ty());
        llvm::ArrayRef<llvm::Type*> print_args_ref(print_args);

        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), print_args_ref, false);

        print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_integer", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_integer_entry", print_def);

        builder->SetInsertPoint(print_entry);

        llvm::Value* format_string = builder->CreateGlobalStringPtr("%d");

        builder->CreateCall2(global_printf, format_string, print_def->arg_begin()++);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def, result);
    }
    else if(result_type->type == CFL_TYPE_CHAR)
    {
        llvm::Function* lookup = top_module->getFunction("__print_char");

        if(lookup)
        {
            builder->CreateCall(lookup, result);

            return;
        }

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

        llvm::Value* format_string = builder->CreateGlobalStringPtr("'%c'");

        builder->CreateCall2(global_printf, format_string, print_def->arg_begin()++);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def, result);
    }
    else if(result_type->type == CFL_TYPE_TUPLE)
    {
        static llvm::Value* open_parentheses = builder->CreateGlobalStringPtr("(");
        static llvm::Value* comma = builder->CreateGlobalStringPtr(", ");
        static llvm::Value* close_parentheses = builder->CreateGlobalStringPtr(")");

        llvm::ArrayType* array_type =
            llvm::ArrayType::get(builder->getInt8PtrTy(), result_type->id);

        std::vector<llvm::Type*> print_args;
        print_args.push_back(array_type);
        llvm::ArrayRef<llvm::Type*> print_args_ref(print_args);

        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), print_args_ref, false);

        llvm::Function* new_print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_tuple", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_tuple_entry", new_print_def);

        builder->SetInsertPoint(print_entry);

        builder->CreateCall(global_printf, open_parentheses);

        llvm::Value* argument = new_print_def->arg_begin()++;

        int i = 0;
        for( ; i < result_type->id; ++i)
        {
            llvm::Value* extraction = builder->CreateExtractValue(argument, i);

            llvm::Value* child = extract_value_from_pointer(
                extraction, ((cfl_type**) result_type->input)[i]);

            generate_print_function(
                ((cfl_type**) result_type->input)[i], child, print_entry);

            if(i < result_type->id - 1)
                builder->CreateCall(global_printf, comma);
        }

        print_def = new_print_def;

        builder->CreateCall(global_printf, close_parentheses);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def, result);
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

        builder->CreateCall(global_printf, success_string);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def);
    }
}

bool cfl_Compiler::compile_program(cfl_typed_program* program)
{
    llvm::FunctionType* main_type = llvm::FunctionType::get(
        builder->getInt32Ty(), false);

    llvm::Function* main_def = llvm::Function::Create(
        main_type, llvm::Function::ExternalLinkage, "main", top_module);

    llvm::BasicBlock* main_entry = llvm::BasicBlock::Create(
        global_context, "main_entry", main_def);

    builder->SetInsertPoint(main_entry);

    setup_global_defs();

    builder->SetInsertPoint(main_entry);

    llvm::Value* result = compile_node(program->main, main_def);

    if(!result)
        return false;

    llvm::BasicBlock* insert_block = builder->GetInsertBlock();

    generate_print_function(program->main->resulting_type, result, insert_block);

    llvm::Value* empty_string = builder->CreateGlobalStringPtr("");

    builder->CreateCall(global_puts, empty_string);
    builder->CreateRet(llvm::ConstantInt::get(builder->getInt32Ty(), 0));

    return true;
}

bool cfl_Compiler::compile(cfl_typed_program* program, std::string& destination_file)
{
    builder = new llvm::IRBuilder<>(global_context);
    top_module = new llvm::Module(destination_file, global_context);

    if(!compile_program(program))
        return false;

    top_module->dump();

    delete builder;
//    delete top_module;

    return true;
}
