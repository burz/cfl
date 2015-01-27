#include "cfl_compiler.h"

extern "C" {
#include "cfl_type.h"
}

#include <iostream>
#include <sstream>

namespace Cfl {

Compiler::Compiler(void)
    : global_context(llvm::getGlobalContext())
{
}

bool Compiler::generate_function_struct_types(
        cfl_typed_node* node,
        argument_type_map saved_argument_types,
        llvm::FunctionType** function_type,
        llvm::StructType** struct_type)
{
    std::vector<llvm::Type*> args;
    argument_type_map new_argument_types;

    argument_type_map::iterator itt = saved_argument_types.begin();
    argument_type_map::iterator end = saved_argument_types.end();

    for( ; itt != end; ++itt)
        if(cfl_is_free_in_typed_node((char*) itt->first->data, node))
        {
            args.push_back(itt->second); 
            new_argument_types.push_back(*itt);
        }

    while(node->node_type == CFL_NODE_FUNCTION)
    {
        if(node->children[0]->resulting_type->type == CFL_TYPE_VARIABLE)
        {
            node = node->children[1];

            continue;
        }

        llvm::Type* input_type =
            generate_type_inner(saved_argument_types, node->children[0]);

        if(!input_type)
            return 0;

        args.push_back(input_type);

        argument_type_mapping mapping(node->children[0], input_type);

        new_argument_types.push_back(mapping);

        node = node->children[1];
    }

    llvm::ArrayRef<llvm::Type*> args_ref(args);
cfl_print_type(node->resulting_type);
    llvm::Type* return_type = generate_type_inner(new_argument_types, node);

    *function_type =
        llvm::FunctionType::get(return_type, args_ref, false);

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

void Compiler::generate_list_struct_types(
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

llvm::Type* Compiler::generate_type_inner(
        argument_type_map type_map,
        cfl_typed_node* node)
{
    if(node->resulting_type->type == CFL_TYPE_BOOL)
        return builder->getInt1Ty();
    else if(node->resulting_type->type == CFL_TYPE_INTEGER)
        return builder->getInt32Ty();
    else if(node->resulting_type->type == CFL_TYPE_CHAR)
        return builder->getInt8Ty();
    else if(node->resulting_type->type == CFL_TYPE_LIST)
    {
        llvm::StructType* struct_type;
        llvm::PointerType* pointer_type;

        generate_list_struct_types(&struct_type, &pointer_type);

        return pointer_type;
    }
    else if(node->resulting_type->type == CFL_TYPE_TUPLE)
        return llvm::ArrayType::get(builder->getInt8PtrTy(),
                                    node->number_of_children);
    else if(node->resulting_type->type == CFL_TYPE_ARROW)
    {
        llvm::FunctionType* function_type;
        llvm::StructType* struct_type;

        if(!generate_function_struct_types(node, type_map, &function_type, &struct_type))
            return 0;

        return struct_type;
    }

    return 0;
}

llvm::Type* Compiler::generate_type(
        argument_register_map register_map,
        cfl_typed_node* node)
{
    argument_type_map type_map;

    argument_register_map::iterator itt = register_map.begin();
    argument_register_map::iterator end = register_map.end();

    for( ; itt != end; ++itt)
    {
        argument_type_mapping mapping(itt->first, itt->second->getType());

        type_map.push_back(mapping);
    }

    return generate_type_inner(type_map, node);
}

llvm::Value* Compiler::compile_node_bool(cfl_typed_node* node)
{
    bool value = *((bool*) node->data);

    return builder->getInt1(value);
}

llvm::Value* Compiler::compile_node_integer(cfl_typed_node* node)
{
    int value = *((int*) node->data);

    return builder->getInt32(value);
}

llvm::Value* Compiler::compile_node_char(cfl_typed_node* node)
{
    char value = *((char*) node->data);

    return builder->getInt8(value);
}

llvm::Value* Compiler::compile_function_chain(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block,
        cfl_typed_node* first_argument)
{
    cfl_typed_node* expression = node;
    std::vector<llvm::Type*> arguments;
    std::vector<cfl_typed_node*> argument_names;

    while(expression->node_type == CFL_NODE_FUNCTION)
    {
        arguments.push_back(generate_type(register_map, expression->children[0]));
        argument_names.push_back(expression->children[0]);

        expression = expression->children[1];
    }

llvm::Type* s_type = generate_type(register_map, node);
std::cout << "\n = s_type";
s_type->dump();
    llvm::Type* return_type = generate_type(register_map, expression);
std::cout << "\n = return_type";
return_type->dump();
std::cout << std::endl;

    argument_register_map::iterator argument_reg_itt = register_map.begin();
    argument_register_map::iterator argument_reg_end = register_map.end();

    for( ; argument_reg_itt != argument_reg_end; ++argument_reg_itt)
        if(cfl_is_free_in_typed_node((char*) argument_reg_itt->first->data, node))
        {
            arguments.insert(arguments.begin(), argument_reg_itt->second->getType());
            argument_names.insert(argument_names.begin(), argument_reg_itt->first);
        }

    llvm::ArrayRef<llvm::Type*> arguments_ref(arguments);

    std::stringstream new_name;
    new_name << "_function_" << (char*) node->children[0]->data << '_' << node;

    llvm::FunctionType* function_type = llvm::FunctionType::get(
        return_type, arguments_ref, false);

    llvm::Function* function_def = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, new_name.str(), top_module);

    llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(
        global_context, "function_entry", function_def);

    builder->SetInsertPoint(function_entry);

    argument_register_map new_register_map;

    std::vector<cfl_typed_node*>::iterator argument_name_itt = argument_names.begin();
    std::vector<cfl_typed_node*>::iterator argument_name_end = argument_names.end();
    llvm::Function::arg_iterator arg_itt = function_def->arg_begin();

    for( ; argument_name_itt != argument_name_end; ++argument_name_itt)
    {
        argument_register_mapping new_mapping(*argument_name_itt, arg_itt++);

        new_register_map.push_back(new_mapping);
    }

    llvm::Value* result =
        compile_node(expression, new_register_map, function_def, function_entry);
std::cout << "\n = result";
result->getType()->dump();
std::cout << std::endl;

    builder->CreateRet(result);

    builder->SetInsertPoint(entry_block);

    llvm::ArrayType* array_type =
        llvm::ArrayType::get(builder->getInt8PtrTy(), register_map.size());

    std::vector<llvm::Type*> members;
    members.push_back(function_def->getType());
    members.push_back(array_type);
    llvm::ArrayRef<llvm::Type*> members_ref(members);

    llvm::StructType* struct_type = llvm::StructType::get(global_context, members_ref);

    std::vector<llvm::Constant*> initial_values;

    for(int i = 0; i < register_map.size(); ++i)
        initial_values.push_back(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));

    llvm::ArrayRef<llvm::Constant*> initial_values_ref(initial_values);

    llvm::Constant* initial_arguments =
        llvm::ConstantArray::get(array_type, initial_values_ref);

    std::vector<llvm::Constant*> function_values;
    function_values.push_back(function_def);
    function_values.push_back(initial_arguments);
    llvm::ArrayRef<llvm::Constant*> function_values_ref(function_values);

    llvm::Constant* initial_struct =
        llvm::ConstantStruct::get(struct_type, function_values_ref);

    llvm::Value* argument_array =
        builder->CreateExtractValue(initial_struct, 1, "argument_array");

    int i = 0;
    argument_reg_itt = register_map.begin();
    for( ; argument_reg_itt != argument_reg_end; ++argument_reg_itt)
        if(cfl_is_free_in_typed_node((char*) argument_reg_itt->first->data, node))
        {
            llvm::AllocaInst* argument_space = builder->CreateAlloca(
                argument_reg_itt->second->getType(), builder->getInt32(1),
                "argument_space");

            builder->CreateStore(argument_reg_itt->second, argument_space);

            llvm::Value* argument_pointer =
                builder->CreatePointerCast(argument_space, builder->getInt8PtrTy());

            argument_array =
                builder->CreateInsertValue(argument_array, argument_pointer, i);

            ++i;
        }

    return builder->CreateInsertValue(initial_struct, argument_array, 1, "happy_array");
}

llvm::Value* Compiler::compile_node_function(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    return compile_function_chain(node, register_map, parent, entry_block);
}

llvm::Value* Compiler::compile_node_list(
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

    llvm::Type* element_type =
        generate_type(register_map, ((cfl_typed_node_list*) node->data)->node);

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

llvm::Value* Compiler::compile_node_tuple(
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

        llvm::Type* child_type = generate_type(register_map, node->children[i]);

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

llvm::Value* Compiler::compile_node_and(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateAnd(left, right, "and");
}

llvm::Value* Compiler::compile_node_or(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateOr(left, right, "or");
}

llvm::Value* Compiler::compile_node_not(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* child = compile_node(node->children[0], register_map, parent, entry_block);

    return builder->CreateNot(child, "not");
}

llvm::Value* Compiler::compile_node_add(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateAdd(left, right, "add");
}

llvm::Value* Compiler::compile_node_multiply(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateMul(left, right, "multiply");
}

llvm::Value* Compiler::compile_node_divide(
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

llvm::Value* Compiler::compile_node_equal(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateICmpEQ(left, right, "equal");
}

llvm::Value* Compiler::compile_node_less(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(node->children[0], register_map, parent, entry_block);
    llvm::Value* right = compile_node(node->children[1], register_map, parent, entry_block);

    return builder->CreateICmpSLT(left, right, "less");
}

llvm::Value* Compiler::compile_node_if(
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
        compile_node(node->children[1], register_map, parent, if_true);

    builder->SetInsertPoint(if_true);

    builder->CreateBr(if_end);

    builder->SetInsertPoint(if_false);

    llvm::Value* else_value =
        compile_node(node->children[2], register_map, parent, if_false);

    builder->SetInsertPoint(if_false);

    builder->CreateBr(if_end);

    builder->SetInsertPoint(if_end);

    llvm::PHINode* phi = builder->CreatePHI(then_value->getType(), 2, "phi");

    phi->addIncoming(then_value, if_true);
    phi->addIncoming(else_value, if_false);

    return phi;
}

llvm::Value* Compiler::compile_node_push(
        cfl_typed_node* node,
       argument_register_map register_map,
       llvm::Function* parent,
       llvm::BasicBlock* entry_block)
{
    llvm::Value* element =
        compile_node(node->children[0], register_map, parent, entry_block);

    llvm::Type* element_type = generate_type(register_map, node->children[0]);

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

llvm::Value* Compiler::compile_node_concatenate(
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

llvm::Value* Compiler::compile_node_case(
        cfl_typed_node* node,
        argument_register_map register_map,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* list =
        compile_node(node->children[0], register_map, parent, entry_block);

    llvm::Value* is_empty = builder->CreateIsNull(list, "is_empty");

    llvm::BasicBlock* empty = llvm::BasicBlock::Create(
        global_context, "__empty", parent);
    llvm::BasicBlock* nonempty = llvm::BasicBlock::Create(
        global_context, "__nonempty", parent);
    llvm::BasicBlock* case_end = llvm::BasicBlock::Create(
        global_context, "__case_end", parent);

    builder->CreateCondBr(is_empty, empty, nonempty);

    builder->SetInsertPoint(empty);

    llvm::Value* empty_result =
        compile_node(node->children[1], register_map, parent, empty);

    builder->CreateBr(case_end);

    builder->SetInsertPoint(nonempty);

    llvm::Value* list_node = builder->CreateLoad(list, "list_node");

    llvm::Value* element_pointer =
        builder->CreateExtractValue(list_node, 0, "element_pointer");

    llvm::Value* element = builder->CreateLoad(element_pointer, "element");
    llvm::Value* tail = builder->CreateExtractValue(list_node, 1, "tail");

    argument_register_mapping element_mapping(node->children[2], element);
    argument_register_mapping tail_mapping(node->children[3], tail);

    register_map.push_back(element_mapping);
    register_map.push_back(tail_mapping);

    llvm::Value* nonempty_result =
        compile_node(node->children[4], register_map, parent, nonempty);

    builder->CreateBr(case_end);

    builder->SetInsertPoint(case_end);

    llvm::Type* result_type = generate_type(register_map, node);

    llvm::PHINode* phi = builder->CreatePHI(result_type, 2, "phi");

    phi->addIncoming(empty_result, empty);
    phi->addIncoming(nonempty_result, nonempty);

    return phi;
}

llvm::Value* Compiler::compile_node(
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
    else if(node->node_type == CFL_NODE_CASE)
        return compile_node_case(node, register_map, parent, entry_block);

    return 0;
}

void Compiler::setup_global_defs(void)
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

llvm::Value* Compiler::extract_value_from_pointer(
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

bool Compiler::compile_program(cfl_typed_program* program)
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

bool Compiler::compile(cfl_typed_program* program, std::string& filename_head)
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

} // end namespace Cfl
