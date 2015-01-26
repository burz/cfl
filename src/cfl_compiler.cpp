#include "cfl_compiler.h"

extern "C" {
#include "cfl_type.h"
}

#include <iostream>
#include <sstream>

CflCompiler::CflCompiler(void)
    : global_context(llvm::getGlobalContext())
{
}

bool CflCompiler::generate_function_struct_types(
        cfl_type* type,
        llvm::FunctionType** function_type,
        llvm::StructType** struct_type)
{
    std::vector<llvm::Type*> args;

    while(type->type == CFL_TYPE_ARROW)
    {
        cfl_type* input = (cfl_type*) type->input;

        if(input->type == CFL_TYPE_VARIABLE)
        {
            type = (cfl_type*) type->output;

            continue;
        }

        llvm::Type* input_type = generate_type(input);

        if(!input_type)
            return 0;

        args.push_back(input_type);

        type = (cfl_type*) type->output;
    }

    llvm::ArrayRef<llvm::Type*> args_ref(args);

    *function_type =
        llvm::FunctionType::get(builder->getInt8PtrTy(), args_ref, false);

    if(!*function_type)
        return 0;

    llvm::PointerType* function_pointer_type =
        llvm::PointerType::getUnqual(*function_type);

    llvm::ArrayType* array_type =
        llvm::ArrayType::get(builder->getInt8PtrTy(), args.size());

    std::vector<llvm::Type*> members;
    members.push_back(function_pointer_type);
    members.push_back(array_type);
    llvm::ArrayRef<llvm::Type*> members_ref(members);

    *struct_type = llvm::StructType::create(members_ref);

    return true;
}

void CflCompiler::generate_list_struct_types(
        llvm::StructType** struct_type,
        llvm::PointerType** struct_pointer_type)
{
    static llvm::StructType* list_type = 0;

    if(list_type)
    {
        *struct_type = list_type;
        *struct_pointer_type = llvm::PointerType::getUnqual(list_type);

        return;
    }

    std::vector<llvm::Type*> members;

    members.push_back(builder->getInt8PtrTy());

    list_type = llvm::StructType::create(global_context, "list_node");

    *struct_pointer_type = llvm::PointerType::getUnqual(list_type);
    members.push_back(llvm::PointerType::getUnqual(list_type));

    llvm::ArrayRef<llvm::Type*> members_ref(members);

    list_type->setBody(members_ref);

    *struct_type = list_type;
}

llvm::Type* CflCompiler::generate_type(cfl_type* type)
{
    if(type->type == CFL_TYPE_BOOL)
        return builder->getInt1Ty();
    else if(type->type == CFL_TYPE_INTEGER)
        return builder->getInt32Ty();
    else if(type->type == CFL_TYPE_CHAR)
        return builder->getInt8Ty();
    else if(type->type == CFL_TYPE_LIST)
    {
        llvm::StructType* struct_type;
        llvm::PointerType* pointer_type;

        generate_list_struct_types(&struct_type, &pointer_type);

        return pointer_type;
    }
    else if(type->type == CFL_TYPE_TUPLE)
        return llvm::ArrayType::get(builder->getInt8PtrTy(), type->id);
    else if(type->type == CFL_TYPE_ARROW)
        return builder->getInt8PtrTy();
//    {
//        llvm::FunctionType* function_type;
//        llvm::StructType* struct_type;
//
//        generate_function_struct_types(type, &function_type, &struct_type);
//
//        return struct_type;
//    }

    return 0;
}

llvm::Value* CflCompiler::compile_node_bool(cfl_typed_node* node)
{
    bool value = *((bool*) node->data);

    return builder->getInt1(value);
}

llvm::Value* CflCompiler::compile_node_integer(cfl_typed_node* node)
{
    int value = *((int*) node->data);

    return builder->getInt32(value);
}

llvm::Value* CflCompiler::compile_node_char(cfl_typed_node* node)
{
    char value = *((char*) node->data);

    return builder->getInt8(value);
}

llvm::Value* CflCompiler::compile_node_function(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    cfl_type* updated_type = cfl_copy_new_type(node->resulting_type);

    if(!updated_type)
        return 0;

    argument_register_map::iterator argument_reg_itt = register_map.begin();
    argument_register_map::iterator argument_reg_end = register_map.end();

    for( ; argument_reg_itt != argument_reg_end; ++argument_reg_itt)
    {
        if(cfl_is_free_in_typed_node((char*) argument_reg_itt->first->data, node))
        {
            cfl_type* argument_type_copy =
                cfl_copy_new_type(argument_reg_itt->first->resulting_type);

            if(!argument_type_copy)
                return 0;

            updated_type = cfl_create_new_type_arrow(argument_type_copy, updated_type);
        }
    }

    llvm::FunctionType* function_type;
    llvm::StructType* function_struct_type;

    if(!generate_function_struct_types(updated_type, &function_type, &function_struct_type))
        return 0;

    cfl_free_type(updated_type);

    std::stringstream new_name;
    new_name << "_function_" << (char*) node->children[0]->data << '_' << node;

    llvm::Function* function_def = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, new_name.str(), top_module);

    llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(
        global_context, "function_entry", function_def);

    argument_register_map new_register_map;
    std::vector<llvm::Argument*> applicable_arguments;

    llvm::Function::arg_iterator arg_itt = function_def->arg_begin();

    argument_reg_itt = register_map.begin();

    for( ; argument_reg_itt != argument_reg_end; ++argument_reg_itt)
        if(cfl_is_free_in_typed_node((char*) argument_reg_itt->first->data, node))
        {
            llvm::Argument* argument = arg_itt++;

            argument_register_mapping mapping(argument_reg_itt->first, argument);

            new_register_map.push_back(mapping);
            applicable_arguments.push_back(argument);
        }

    cfl_typed_node* pos = node;

    while(pos->node_type == CFL_NODE_FUNCTION)
    {
        llvm::Argument* argument = arg_itt++;

        argument_register_mapping mapping(pos->children[0], argument);

        new_register_map.push_back(mapping);
        applicable_arguments.push_back(argument);

        pos = pos->children[1];
    }

    builder->SetInsertPoint(function_entry);

    llvm::Value* result =
        compile_node(pos, new_register_map, function_def, function_entry);

    if(!result)
        return 0;
result->dump();

//    if(pos->resulting_type->type != CFL_TYPE_ARROW)
//    {
//        llvm::Value* function_struct = builder->CreateLoad(result);
//
//        llvm::Value* function_ptr = builder->CreateExtractValue(function_struct, 0);
//
//        llvm::ArrayRef<llvm::Value*> applicable_arguments_ref(applicable_arguments);
//
//        builder->CreateCall(function_ptr, applicable_arguments_ref);
//    }

    llvm::Type* result_type = generate_type(pos->resulting_type);

    llvm::AllocaInst* result_space =
        builder->CreateAlloca(result_type, builder->getInt32(1), "result_space");

    builder->CreateStore(result, result_space);

    llvm::Value* result_pointer =
        builder->CreatePointerCast(result_space, builder->getInt8PtrTy());

    builder->CreateRet(result_pointer);

    builder->SetInsertPoint(entry_block);

    std::vector<llvm::Constant*> initial_values;

    for(int i = 0; i < register_map.size(); ++i)
        initial_values.push_back(
            llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));

    llvm::ArrayRef<llvm::Constant*> initial_values_ref(initial_values);

    llvm::Constant* arguments = llvm::ConstantArray::get(
        llvm::cast<llvm::ArrayType>(function_struct_type->getElementType(1)),
        initial_values_ref);

    std::vector<llvm::Constant*> function_values;
    function_values.push_back(function_def);
    function_values.push_back(arguments);
    llvm::ArrayRef<llvm::Constant*> function_values_ref(function_values);

    return llvm::ConstantStruct::get(function_struct_type, function_values_ref);
}

llvm::Value* CflCompiler::compile_node_list(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::StructType* list_type;
    llvm::PointerType* list_pointer_type;

    generate_list_struct_types(&list_type, &list_pointer_type);

    if(!node->data)
        return llvm::ConstantPointerNull::get(list_pointer_type);

    llvm::Type* element_type = generate_type((cfl_type*) node->resulting_type->input);

    cfl_typed_node_list* pos = (cfl_typed_node_list*) node->data;

    llvm::Value* head = 0;
    llvm::Value* last_node = 0;

    while(pos)
    {
        llvm::Value* element =
            compile_node(pos->node, register_map, parent, entry_block);

        if(!element)
            return 0;

        llvm::Constant* size = builder->getInt32(1);

        llvm::AllocaInst* element_space =
            builder->CreateAlloca(element_type, size, "element_space");

        builder->CreateStore(element, element_space);

        llvm::Value* element_pointer =
            builder->CreatePointerCast(element_space, builder->getInt8PtrTy());

        std::vector<llvm::Constant*> initial_values;
        initial_values.push_back(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
        initial_values.push_back(llvm::ConstantPointerNull::get(list_pointer_type));
        llvm::ArrayRef<llvm::Constant*> initial_values_ref(initial_values);

        llvm::Value* node = llvm::ConstantStruct::get(list_type, initial_values_ref);

        llvm::Value* store =
            builder->CreateInsertValue(node, element_pointer, 0, "store");

        llvm::AllocaInst* node_space =
            builder->CreateAlloca(list_type, size, "node_space");

        builder->CreateStore(store, node_space);

        if(last_node)
        {
            llvm::Value* local_last_node =
                builder->CreateLoad(last_node, "local_last_node");

            llvm::Value* new_last_node =
                builder->CreateInsertValue(local_last_node, node_space, 1);

            builder->CreateStore(new_last_node, last_node);
        }
        else
            head = node_space;

        last_node = node_space;

        pos = pos->next;
    }

    return head;
}

llvm::Value* CflCompiler::compile_node_tuple(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    std::vector<llvm::Value*> tuple;
    std::vector<llvm::Constant*> zeroes;

    int i = 0;
    for( ; i < node->number_of_children; ++i)
    {
        llvm::Value* child =
            compile_node(node->children[i], register_map, parent, entry_block);

        if(!child)
            return 0;

        llvm::Type* child_type =
            generate_type(((cfl_type**) node->resulting_type->input)[i]);

        if(!child_type)
            return 0;

        llvm::Constant* size = builder->getInt32(1);

        llvm::AllocaInst* child_space =
            builder->CreateAlloca(child_type, size, "child_space");

        builder->CreateStore(child, child_space);

        llvm::Value* child_pointer = builder->CreatePointerCast(
            child_space, builder->getInt8PtrTy(), "child_pointer");

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

llvm::Value* CflCompiler::compile_node_and(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateAnd(left, right, "and");
}

llvm::Value* CflCompiler::compile_node_or(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateOr(left, right, "or");
}

llvm::Value* CflCompiler::compile_node_not(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* child = compile_node(node->children[0], register_map, parent, entry_block);

    return builder->CreateNot(child, "not");
}

llvm::Value* CflCompiler::compile_node_add(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateAdd(left, right, "add");
}

llvm::Value* CflCompiler::compile_node_multiply(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateMul(left, right, "multiply");
}

llvm::Value* CflCompiler::compile_node_divide(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    static llvm::Value* error_division_by_zero_string =
        builder->CreateGlobalStringPtr("EVALUATION ERROR: Division by zero");

    llvm::Value* right =
        compile_node(node->children[1], register_map, parent, entry_block);

    llvm::Value* is_zero =
        builder->CreateICmpEQ(right, builder->getInt32(0), "is_zero");

    llvm::BasicBlock* zero = llvm::BasicBlock::Create(
        global_context, "__zero", parent);
    llvm::BasicBlock* nonzero = llvm::BasicBlock::Create(
        global_context, "__nonzero", parent);

    builder->CreateCondBr(is_zero, zero, nonzero);

    builder->SetInsertPoint(zero);

    builder->CreateCall(global_puts, error_division_by_zero_string);

    std::vector<llvm::Type*> args;
    args.push_back(builder->getInt32Ty());
    llvm::ArrayRef<llvm::Type*> args_ref(args);

    llvm::FunctionType* exit_type =
        llvm::FunctionType::get(builder->getVoidTy(), args_ref, false);

    llvm::Value* exit = top_module->getOrInsertFunction("exit", exit_type);

    builder->CreateCall(exit, builder->getInt32(1));

    builder->CreateUnreachable();

    builder->SetInsertPoint(nonzero);

    llvm::Value* left =
        compile_node(node->children[0], register_map, parent, entry_block);

    return builder->CreateSDiv(left, right, "divide");
}

llvm::Value* CflCompiler::compile_node_equal(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateICmpEQ(left, right, "equal");
}

llvm::Value* CflCompiler::compile_node_less(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateICmpSLT(left, right, "less");
}

llvm::Value* CflCompiler::compile_node_if(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* condition = compile_node(node->children[0], register_map, parent, entry_block);

    llvm::BasicBlock* if_true = llvm::BasicBlock::Create(
        global_context, "__if_true", parent);
    llvm::BasicBlock* if_false = llvm::BasicBlock::Create(
        global_context, "__if_false", parent);
    llvm::BasicBlock* if_end = llvm::BasicBlock::Create(
        global_context, "__if_end", parent);

    builder->CreateCondBr(condition, if_true, if_false);

    builder->SetInsertPoint(if_true);

    llvm::Value* then_value =
        compile_node(node->children[1], register_map, parent, entry_block);

    builder->SetInsertPoint(if_true);

    builder->CreateBr(if_end);

    builder->SetInsertPoint(if_false);

    llvm::Value* else_value =
        compile_node(node->children[2], register_map, parent, entry_block);

    builder->SetInsertPoint(if_false);

    builder->CreateBr(if_end);

    builder->SetInsertPoint(if_end);

    llvm::PHINode* phi = builder->CreatePHI(then_value->getType(), 2, "phi");

    phi->addIncoming(then_value, if_true);
    phi->addIncoming(else_value, if_false);

    return phi;
}

llvm::Value* CflCompiler::compile_node_push(
        cfl_typed_node* node,
       argument_register_map register_map,
       llvm::Function* parent,
       llvm::BasicBlock* entry_block)
{
    llvm::Value* element =
        compile_node(node->children[0], register_map, parent, entry_block);

    llvm::Type* element_type = generate_type(node->children[0]->resulting_type);

    llvm::Value* element_space = builder->CreateAlloca(element_type);

    builder->CreateStore(element, element_space, "stored_element");

    llvm::Value* element_pointer = builder->CreatePointerCast(
        element_space, builder->getInt8PtrTy(), "element_pointer");

    llvm::Value* tail =
        compile_node(node->children[1], register_map, parent, entry_block);

    llvm::StructType* list_type;
    llvm::PointerType* list_pointer_type;

    generate_list_struct_types(&list_type, &list_pointer_type);

    std::vector<llvm::Constant*> initial_values;
    initial_values.push_back(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    initial_values.push_back(llvm::ConstantPointerNull::get(list_pointer_type));
    llvm::ArrayRef<llvm::Constant*> initial_values_ref(initial_values);

    llvm::Value* initial_node = llvm::ConstantStruct::get(list_type, initial_values_ref);

    llvm::Value* happy_node =
        builder->CreateInsertValue(initial_node, element_pointer, 0, "happy_node");

    llvm::Value* connected_node =
        builder->CreateInsertValue(happy_node, tail, 1, "connected_node");

    llvm::AllocaInst* list_node_space =
        builder->CreateAlloca(list_type, builder->getInt32(1), "list_node_space");

    builder->CreateStore(connected_node, list_node_space);

    return list_node_space;
}

llvm::Value* CflCompiler::compile_node_concatenate(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left =
        compile_node(node->children[0], register_map, parent, entry_block);

    llvm::Value* right =
        compile_node(node->children[1], register_map, parent, entry_block);

    llvm::Value* is_right_null = builder->CreateIsNull(right, "is_right_null");

    llvm::BasicBlock* is_right_null_true = llvm::BasicBlock::Create(
        global_context, "__is_right_null_true", parent);
    llvm::BasicBlock* is_right_null_false = llvm::BasicBlock::Create(
        global_context, "__is_right_null_false", parent);
    llvm::BasicBlock* is_left_null_false = llvm::BasicBlock::Create(
        global_context, "__is_left_null_false", parent);
    llvm::BasicBlock* list_loop = llvm::BasicBlock::Create(
        global_context, "__list_loop", parent);
    llvm::BasicBlock* list_loop_end = llvm::BasicBlock::Create(
        global_context, "__list_loop_end", parent);
    llvm::BasicBlock* concatenate_end = llvm::BasicBlock::Create(
        global_context, "__concatenate_end", parent);

    builder->CreateCondBr(is_right_null, is_right_null_true, is_right_null_false);

    builder->SetInsertPoint(is_right_null_true);

    builder->CreateBr(concatenate_end);

    builder->SetInsertPoint(is_right_null_false);

    llvm::Value* is_left_null = builder->CreateIsNull(left, "is_left_null");

    builder->CreateCondBr(is_left_null, concatenate_end, is_left_null_false);

    builder->SetInsertPoint(is_left_null_false);

    llvm::StructType* list_type;
    llvm::PointerType* list_pointer_type;

    generate_list_struct_types(&list_type, &list_pointer_type);

    llvm::AllocaInst* node_space = builder->CreateAlloca(list_pointer_type);

    builder->CreateStore(left, node_space);

    builder->CreateBr(list_loop);

    builder->SetInsertPoint(list_loop);

    llvm::LoadInst* node_pointer = builder->CreateLoad(node_space, "node_pointer");

    llvm::LoadInst* list_node = builder->CreateLoad(node_pointer, "node");

    llvm::Value* next_pointer =
        builder->CreateExtractValue(list_node, 1, "next_pointer");

    builder->CreateStore(next_pointer, node_space);

    llvm::Value* is_next_null =
        builder->CreateIsNull(next_pointer, "is_next_null");

    builder->CreateCondBr(is_next_null, list_loop_end, list_loop);

    builder->SetInsertPoint(list_loop_end);

    llvm::Value* connected_list =
        builder->CreateInsertValue(list_node, right, 1, "connected_list");

    builder->CreateStore(connected_list, node_pointer);

    builder->CreateBr(concatenate_end);

    builder->SetInsertPoint(concatenate_end);

    llvm::PHINode* phi = builder->CreatePHI(list_pointer_type, 3, "phi");

    phi->addIncoming(left, is_right_null_true);
    phi->addIncoming(right, is_right_null_false);
    phi->addIncoming(left, list_loop_end);

    return phi;
}

llvm::Value* CflCompiler::compile_node(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    if(node->node_type == CFL_NODE_VARIABLE)
    {
        argument_register_map::iterator itt = register_map.begin();
        argument_register_map::iterator end = register_map.end();

        for( ; itt != end; ++itt)
            if(!strcmp((char*) itt->first->data, (char*) node->data))
                return itt->second;
    }
    else if(node->node_type == CFL_NODE_BOOL)
        return compile_node_bool(node);
    else if(node->node_type == CFL_NODE_INTEGER)
        return compile_node_integer(node);
    else if(node->node_type == CFL_NODE_CHAR)
        return compile_node_char(node);
    else if(node->node_type == CFL_NODE_FUNCTION)
        return compile_node_function(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_LIST)
        return compile_node_list(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_TUPLE)
        return compile_node_tuple(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_AND)
        return compile_node_and(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_OR)
        return compile_node_or(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_NOT)
        return compile_node_not(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_ADD)
        return compile_node_add(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_MULTIPLY)
        return compile_node_multiply(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_DIVIDE)
        return compile_node_divide(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_EQUAL)
        return compile_node_equal(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_LESS)
        return compile_node_less(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_IF)
        return compile_node_if(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_PUSH)
        return compile_node_push(node, register_map, parent, entry_block);
    else if(node->node_type == CFL_NODE_CONCATENATE)
        return compile_node_concatenate(node, register_map, parent, entry_block);

    return 0;
}

void CflCompiler::setup_global_defs(void)
{
    std::vector<llvm::Type*> puts_args;
    puts_args.push_back(builder->getInt8Ty()->getPointerTo());
    llvm::ArrayRef<llvm::Type*> puts_args_ref(puts_args);

    llvm::FunctionType* puts_type =
        llvm::FunctionType::get(builder->getInt32Ty(), puts_args_ref, false);

    global_puts = top_module->getOrInsertFunction("puts", puts_type);

    std::vector<llvm::Type*> printf_args;
    printf_args.push_back(builder->getInt8Ty()->getPointerTo());
    llvm::ArrayRef<llvm::Type*> printf_args_ref(printf_args);

    llvm::FunctionType* printf_type =
        llvm::FunctionType::get(builder->getInt32Ty(), printf_args_ref, true);

    global_printf = top_module->getOrInsertFunction("printf", printf_type);
}

llvm::Value* CflCompiler::extract_value_from_pointer(
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
    else if(type->type == CFL_TYPE_LIST)
    {
        llvm::StructType* list_type;
        llvm::PointerType* list_pointer_type;

        generate_list_struct_types(&list_type, &list_pointer_type);

        llvm::PointerType* list_pointer_pointer_type =
            llvm::PointerType::getUnqual(list_pointer_type);

        llvm::Value* list_pointer_pointer =
            builder->CreatePointerCast(pointer, list_pointer_pointer_type);

        return builder->CreateLoad(list_pointer_pointer, "load_list");
    }
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

bool CflCompiler::compile_program(cfl_typed_program* program)
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

    argument_register_map register_map;

    llvm::Value* result = compile_node(program->main, register_map, main_def, main_entry);

    if(!result)
        return false;

    llvm::BasicBlock* insert_block = builder->GetInsertBlock();

    generate_print_function(program->main->resulting_type, result, insert_block);

    llvm::Value* empty_string = builder->CreateGlobalStringPtr("");

    builder->CreateCall(global_puts, empty_string);
    builder->CreateRet(llvm::ConstantInt::get(builder->getInt32Ty(), 0));

    return true;
}

bool CflCompiler::compile(cfl_typed_program* program, std::string& filename_head)
{
    builder = new llvm::IRBuilder<>(global_context);
    top_module = new llvm::Module(filename_head + ".cfl", global_context);

    if(!compile_program(program))
        return false;

    top_module->dump();

    delete builder;
// TODO:
//    delete top_module;

    return true;
}
