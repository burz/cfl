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

llvm::Function* Compiler::create_application_function(
        std::string name,
        llvm::ArrayType* array_type,
        llvm::StructType* struct_type,
        llvm::FunctionType* application_type,
        llvm::FunctionType* function_type,
        llvm::Constant* function_def)
{
    std::stringstream new_application_name;
    new_application_name << name << "_application";

    llvm::Function* application_def = llvm::Function::Create(
        application_type, llvm::Function::ExternalLinkage,
        new_application_name.str(), top_module);

    llvm::BasicBlock* application_entry = llvm::BasicBlock::Create(
        global_context, "function_entry", application_def);

    builder->SetInsertPoint(application_entry);

    llvm::Function::arg_iterator itt = application_def->arg_begin();

    llvm::Value* function_pointer = itt++;
    llvm::Value* array_pointer = itt++;
    llvm::Value* argument = itt++;

    std::vector<llvm::Value*> arguments;

    if(function_type->getNumParams() > 1)
    {
        llvm::Value* argument_array_pointer = builder->CreatePointerCast(
            array_pointer, array_type->getPointerTo(), "argument_array_pointer");

        for(int i = 0; i < function_type->getNumParams() - 1; ++i)
        {
            std::vector<llvm::Value*> indices;
            indices.push_back(builder->getInt32(0));
            indices.push_back(builder->getInt32(i));
            llvm::ArrayRef<llvm::Value*> indices_ref(indices);

            llvm::Value* element_pointer = builder->CreateGEP(
                argument_array_pointer, indices_ref, "element_pointer");

            llvm::Value* argument_space =
                builder->CreateLoad(element_pointer, "argument_space");

            llvm::Value* argument_pointer = builder->CreatePointerCast(
                argument_space, function_type->getParamType(i)->getPointerTo());

            llvm::Value* argument = builder->CreateLoad(argument_pointer);

            arguments.push_back(argument);
        }
    }

    arguments.push_back(argument);

    llvm::ArrayRef<llvm::Value*> arguments_ref(arguments);

    llvm::Value* function = builder->CreatePointerCast(
        function_pointer, function_type->getPointerTo(), "function");

    llvm::Value* applied_result = builder->CreateCall(
        function, arguments_ref, "applied_result");

    builder->CreateRet(applied_result);

    return application_def;
}

llvm::Value* Compiler::populate_function_struct(
        argument_register_map register_map,
        cfl_typed_node* node,
        llvm::ArrayType* array_type,
        llvm::StructType* struct_type,
        llvm::Constant* application_def,
        llvm::Constant* function_def,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    std::vector<llvm::Constant*> initial_values;

    for(int i = 0; i < register_map.size(); ++i)
        initial_values.push_back(
            llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));

    llvm::ArrayRef<llvm::Constant*> initial_values_ref(initial_values);

    llvm::Constant* initial_arguments =
        llvm::ConstantArray::get(array_type, initial_values_ref);

    std::vector<llvm::Constant*> function_values;
    function_values.push_back(application_def);
    function_values.push_back(
        llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    function_values.push_back(
        llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    llvm::ArrayRef<llvm::Constant*> function_values_ref(function_values);

    llvm::Constant* initial_struct =
        llvm::ConstantStruct::get(struct_type, function_values_ref);

    llvm::Value* function_pointer =
        builder->CreatePointerCast(function_def, builder->getInt8PtrTy());

    llvm::Value* function_struct = builder->CreateInsertValue(
        initial_struct, function_pointer, 1, "function_struct");

    llvm::Value* arguments_pointer = call_malloc(array_type, parent, entry_block);

    llvm::Value* arguments_space = builder->CreatePointerCast(
        arguments_pointer, array_type->getPointerTo(), "arguments_space");

    builder->CreateStore(initial_arguments, arguments_space);

    int i = 0;
    argument_register_map::iterator argument_reg_itt = register_map.begin();
    argument_register_map::iterator argument_reg_end = register_map.end();

    for( ; argument_reg_itt != argument_reg_end; ++argument_reg_itt)
        if(cfl_is_free_in_typed_node((char*) argument_reg_itt->first->data, node))
        {
            llvm::Type* arg_type = argument_reg_itt->second->getType();

            llvm::Value* arg_pointer = call_malloc(arg_type, parent, entry_block);

            llvm::Value* arg_space = builder->CreatePointerCast(
                arg_pointer, arg_type->getPointerTo(), "arg_space");

            builder->CreateStore(argument_reg_itt->second, arg_space);

            std::vector<llvm::Value*> offsets;
            offsets.push_back(builder->getInt32(0));
            offsets.push_back(builder->getInt32(i));
            llvm::ArrayRef<llvm::Value*> offsets_ref(offsets);

            llvm::Value* struct_location = builder->CreateGEP(
                arguments_space, offsets_ref, "struct_location");

            builder->CreateStore(arg_pointer, struct_location);

            ++i;
        }

    llvm::Value* allocated_struct = builder->CreateInsertValue(
        function_struct, arguments_pointer, 2, "allocated_struct");

    return allocated_struct;
}

void Compiler::add_arguments(
        cfl_typed_node* argument,
        llvm::Value* location,
        argument_register_map& register_map,
        function_map functions)
{
    if(argument->node_type == CFL_NODE_VARIABLE && *((char*) argument->data) != '_')
    {
        argument_register_mapping new_mapping(argument, location);
        register_map.push_back(new_mapping);
    }
    else
        for(int i = 0; i < argument->resulting_type->id; ++i)
        {
            llvm::Value* element_space =
                builder->CreateExtractValue(location, i, "element_space");

            llvm::Type* element_type =
                generate_type(register_map, functions, argument->children[i]);

            llvm::Value* element_pointer = builder->CreatePointerCast(
                element_space, element_type->getPointerTo(), "element_pointer");

            llvm::Value* tuple_element =
                builder->CreateLoad(element_pointer, "tuple_element");

            add_arguments(argument->children[i], tuple_element, register_map, functions);
        }
}

llvm::Value* Compiler::compile_node_function(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    argument_type_map type_map;

    argument_register_map::iterator argument_reg_itt = register_map.begin();
    argument_register_map::iterator argument_reg_end = register_map.end();

    for( ; argument_reg_itt != argument_reg_end; ++argument_reg_itt)
        if(cfl_is_free_in_typed_node((char*) argument_reg_itt->first->data, node))
        {
            argument_type_mapping mapping(argument_reg_itt->first,
                                           argument_reg_itt->second->getType());
            type_map.push_back(mapping);
        }

    llvm::Type* argument_type =
        generate_type(register_map, functions, node->children[0]);

    if(!argument_type)
        return 0;

    argument_type_mapping mapping(node->children[0], argument_type);
    type_map.push_back(mapping);

    llvm::FunctionType* application_type;
    llvm::FunctionType* function_type;
    llvm::ArrayType* array_type;
    llvm::StructType* struct_type;

    if(!generate_function_struct_types(
            node->children[0], node->children[1], type_map,
            functions, &application_type, &function_type,
            &array_type, &struct_type))
        return 0;

    std::stringstream new_name;

    if(node->children[0]->node_type == CFL_NODE_VARIABLE)
        new_name << "_function_" << (char*) node->children[0]->data;
    else
        new_name << "_function_tuple";

    llvm::Function* function_def = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, new_name.str(), top_module);

    llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(
        global_context, "function_entry", function_def);

    builder->SetInsertPoint(function_entry);

    argument_register_map new_register_map;

    argument_type_map::iterator argument_type_itt = type_map.begin();
    argument_type_map::iterator argument_type_end = type_map.end();
    llvm::Function::arg_iterator arg_itt = function_def->arg_begin();

    for( ; argument_type_itt != argument_type_end; ++argument_type_itt)
        add_arguments(argument_type_itt->first, arg_itt++, new_register_map, functions);

    llvm::Value* result = compile_node(
        node->children[1], new_register_map, functions, function_def, function_entry);

    if(!result)
        return 0;

    builder->CreateRet(result);

    llvm::Function* application_def = create_application_function(
        new_name.str(), array_type, struct_type,
        application_type, function_type, function_def);

    builder->SetInsertPoint(entry_block);

    return populate_function_struct(
        register_map, node, array_type, struct_type,
        application_def, function_def, parent, entry_block);
}

llvm::Value* Compiler::compile_node_list(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::StructType* list_type;
    llvm::PointerType* list_pointer_type;

    generate_list_struct_types(&list_type, &list_pointer_type);

    if(!node->data)
        return llvm::ConstantPointerNull::get(list_pointer_type);

    llvm::Type* element_type =
        generate_type(register_map, functions, ((cfl_typed_node_list*) node->data)->node);

    if(!element_type)
        return 0;

    cfl_typed_node_list* pos = (cfl_typed_node_list*) node->data;

    llvm::Value* head = 0;
    llvm::Value* last_node = 0;

    while(pos)
    {
        llvm::Value* element = compile_node(
            pos->node, register_map, functions, parent, entry_block);

        if(!element)
            return 0;

        llvm::Value* element_space = call_malloc(element_type, parent, entry_block);

        llvm::Value* element_pointer = builder->CreatePointerCast(
            element_space, element_type->getPointerTo(), "element_pointer");

        builder->CreateStore(element, element_pointer);

        std::vector<llvm::Constant*> initial_values;
        initial_values.push_back(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
        initial_values.push_back(llvm::ConstantPointerNull::get(list_pointer_type));
        llvm::ArrayRef<llvm::Constant*> initial_values_ref(initial_values);

        llvm::Value* node = llvm::ConstantStruct::get(list_type, initial_values_ref);

        llvm::Value* store =
            builder->CreateInsertValue(node, element_space, 0, "store");

        llvm::Value* node_space = call_malloc(list_type, parent, entry_block);

        llvm::Value* node_pointer =
            builder->CreatePointerCast(node_space, list_pointer_type, "node_pointer");

        builder->CreateStore(store, node_pointer);

        if(last_node)
        {
            llvm::Value* local_last_node =
                builder->CreateLoad(last_node, "local_last_node");

            llvm::Value* new_last_node =
                builder->CreateInsertValue(local_last_node, node_pointer, 1);

            builder->CreateStore(new_last_node, last_node);
        }
        else
            head = node_pointer;

        last_node = node_pointer;

        pos = pos->next;
    }

    return head;
}

llvm::Value* Compiler::compile_node_tuple(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    std::vector<llvm::Value*> tuple;
    std::vector<llvm::Constant*> zeroes;

    int i = 0;
    for( ; i < node->number_of_children; ++i)
    {
        llvm::Value* child = compile_node(
            node->children[i], register_map, functions, parent, entry_block);

        if(!child)
            return 0;

        llvm::Type* child_type =
            generate_type(register_map, functions, node->children[i]);

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
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

    return builder->CreateAnd(left, right, "and");
}

llvm::Value* Compiler::compile_node_or(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

    return builder->CreateOr(left, right, "or");
}

llvm::Value* Compiler::compile_node_not(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* child = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!child)
        return 0;

    return builder->CreateNot(child, "not");
}

llvm::Value* Compiler::compile_node_add(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

    return builder->CreateAdd(left, right, "add");
}

llvm::Value* Compiler::compile_node_multiply(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

    return builder->CreateMul(left, right, "multiply");
}

llvm::Value* Compiler::compile_node_divide(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    static llvm::Value* error_division_by_zero_string =
        builder->CreateGlobalStringPtr("EVALUATION ERROR: Division by zero");

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

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

    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    return builder->CreateSDiv(left, right, "divide");
}

llvm::Value* Compiler::compile_node_equal(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

    return builder->CreateICmpEQ(left, right, "equal");
}

llvm::Value* Compiler::compile_node_less(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

    return builder->CreateICmpSLT(left, right, "less");
}

llvm::Value* Compiler::compile_node_application(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* value = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!value)
        return 0;

    llvm::Value* function_struct = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!function_struct)
        return 0;

    llvm::Value* application_def = builder->CreateExtractValue(
        function_struct, 0, "application_def");

    llvm::Value* function_pointer = builder->CreateExtractValue(
        function_struct, 1, "function_pointer");

    llvm::Value* array_pointer = builder->CreateExtractValue(
        function_struct, 2, "array_pointer");

    return builder->CreateCall3(
        application_def, function_pointer, array_pointer, value, "application_result");
}

llvm::Value* Compiler::compile_node_if(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* condition = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!condition)
        return 0;

    llvm::BasicBlock* if_true = llvm::BasicBlock::Create(
        global_context, "__if_true", parent);
    llvm::BasicBlock* if_false = llvm::BasicBlock::Create(
        global_context, "__if_false", parent);
    llvm::BasicBlock* if_end = llvm::BasicBlock::Create(
        global_context, "__if_end", parent);

    builder->CreateCondBr(condition, if_true, if_false);

    builder->SetInsertPoint(if_true);

    llvm::Value* then_value = compile_node(
        node->children[1], register_map, functions, parent, if_true);

    if(!then_value)
        return 0;

    builder->CreateBr(if_end);

    llvm::BasicBlock* if_true_end = builder->GetInsertBlock();

    builder->SetInsertPoint(if_false);

    llvm::Value* else_value = compile_node(
        node->children[2], register_map, functions, parent, if_false);

    if(!else_value)
        return 0;

    builder->CreateBr(if_end);

    llvm::BasicBlock* if_false_end = builder->GetInsertBlock();

    builder->SetInsertPoint(if_end);

    llvm::PHINode* phi = builder->CreatePHI(then_value->getType(), 2, "phi");

    phi->addIncoming(then_value, if_true_end);
    phi->addIncoming(else_value, if_false_end);

    return phi;
}

llvm::Value* Compiler::compile_node_let_rec(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    argument_type_map type_map;

    argument_register_map::iterator argument_reg_itt = register_map.begin();
    argument_register_map::iterator argument_reg_end = register_map.end();

    for( ; argument_reg_itt != argument_reg_end; ++argument_reg_itt)
    {
        char* argument = (char*) argument_reg_itt->first->data;

        if(strcmp(argument, (char*) node->children[0]->data) &&
           is_not_bound_in(argument_reg_itt->first, node->children[1]) &&
           cfl_is_free_in_typed_node(argument, node->children[2]))
        {
            argument_type_mapping mapping(argument_reg_itt->first,
                                          argument_reg_itt->second->getType());
            type_map.push_back(mapping);
        }
    }

    llvm::Type* argument_type =
        generate_type(register_map, functions, node->children[1]);

    if(!argument_type)
        return 0;

    argument_type_mapping mapping(node->children[1], argument_type);
    type_map.push_back(mapping);

    llvm::FunctionType* application_type;
    llvm::FunctionType* function_type;
    llvm::ArrayType* array_type;
    llvm::StructType* struct_type;

    if(!generate_function_struct_types(
            node->children[1], node->children[2], type_map,
            functions, &application_type, &function_type,
            &array_type, &struct_type))
        return 0;

    std::stringstream new_name;
    new_name << "_let_rec_" << (char*) node->children[0]->data;

    if(node->children[1]->node_type == CFL_NODE_VARIABLE)
        new_name << "_" << (char*) node->children[1]->data;
    else
        new_name << "_tuple";

    llvm::Function* function_def = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, new_name.str(), top_module);

    llvm::Function* application_def = create_application_function(
        new_name.str(), array_type, struct_type,
        application_type, function_type, function_def);

    function_map_result map_result;
    map_result.node = node;
    map_result.array_type = array_type;
    map_result.struct_type = struct_type;
    map_result.application_def = application_def;
    map_result.function_def = function_def;

    function_mapping new_function_mapping((char*) node->children[0]->data, map_result);
    functions.push_back(new_function_mapping);

    llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(
        global_context, "function_entry", function_def);

    builder->SetInsertPoint(function_entry);

    argument_register_map new_register_map;

    argument_type_map::iterator argument_type_itt = type_map.begin();
    argument_type_map::iterator argument_type_end = type_map.end();
    llvm::Function::arg_iterator arg_itt = function_def->arg_begin();

    for( ; argument_type_itt != argument_type_end; ++argument_type_itt)
        add_arguments(argument_type_itt->first, arg_itt++, new_register_map, functions);

    llvm::Value* result = compile_node(
        node->children[2], new_register_map, functions, function_def, function_entry);

    if(!result)
        return 0;

    builder->CreateRet(result);

    builder->SetInsertPoint(entry_block);

    llvm::Value* in_result = compile_node(
        node->children[3], register_map, functions, parent, entry_block);

    functions.pop_back();

    return in_result;
}

llvm::Value* Compiler::compile_node_push(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* element = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!element)
        return 0;

    llvm::Type* element_type =
        generate_type(register_map, functions, node->children[0]);

    if(!element_type)
        return 0;

    llvm::Value* element_pointer = call_malloc(element_type, parent, entry_block);

    llvm::Value* element_space = builder->CreatePointerCast(
        element_pointer, element_type->getPointerTo());

    builder->CreateStore(element, element_space, "stored_element");

    llvm::Value* tail = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!tail)
        return 0;

    llvm::StructType* list_type;
    llvm::PointerType* list_pointer_type;

    generate_list_struct_types(&list_type, &list_pointer_type);

    std::vector<llvm::Constant*> initial_values;
    initial_values.push_back(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    initial_values.push_back(llvm::ConstantPointerNull::get(list_pointer_type));
    llvm::ArrayRef<llvm::Constant*> initial_values_ref(initial_values);

    llvm::Value* initial_node = llvm::ConstantStruct::get(list_type, initial_values_ref);

    llvm::Value* valued_node =
        builder->CreateInsertValue(initial_node, element_pointer, 0, "valued_node");

    llvm::Value* connected_node =
        builder->CreateInsertValue(valued_node, tail, 1, "connected_node");

    llvm::Value* list_node_pointer =
        call_malloc(list_type, parent, builder->GetInsertBlock());

    llvm::Value* list_node_space =
        builder->CreatePointerCast(list_node_pointer, list_type->getPointerTo());

    builder->CreateStore(connected_node, list_node_space);

    return list_node_space;
}

llvm::Value* Compiler::compile_node_concatenate(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* left = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!left)
        return 0;

    llvm::Value* right = compile_node(
        node->children[1], register_map, functions, parent, entry_block);

    if(!right)
        return 0;

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
        function_map functions,
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Value* list = compile_node(
        node->children[0], register_map, functions, parent, entry_block);

    if(!list)
        return 0;

    llvm::Value* is_empty = builder->CreateIsNull(list, "is_empty");

    llvm::BasicBlock* empty = llvm::BasicBlock::Create(
        global_context, "__empty", parent);
    llvm::BasicBlock* nonempty = llvm::BasicBlock::Create(
        global_context, "__nonempty", parent);
    llvm::BasicBlock* case_end = llvm::BasicBlock::Create(
        global_context, "__case_end", parent);

    builder->CreateCondBr(is_empty, empty, nonempty);

    builder->SetInsertPoint(empty);

    llvm::Value* empty_result = compile_node(
        node->children[1], register_map, functions, parent, empty);

    if(!empty_result)
        return 0;

    builder->CreateBr(case_end);

    llvm::BasicBlock* empty_end = builder->GetInsertBlock();

    builder->SetInsertPoint(nonempty);

    llvm::Value* list_node = builder->CreateLoad(list, "list_node");

    llvm::Value* element_pointer =
        builder->CreateExtractValue(list_node, 0, "element_pointer");

    cfl_type* element_type = (cfl_type*) node->children[0]->resulting_type->input;

    llvm::Value* element =
        extract_value_from_pointer(element_pointer, element_type);

    llvm::Value* tail = builder->CreateExtractValue(list_node, 1, "tail");

    argument_register_mapping element_mapping(node->children[2], element);
    argument_register_mapping tail_mapping(node->children[3], tail);

    register_map.push_back(element_mapping);
    register_map.push_back(tail_mapping);

    llvm::Value* nonempty_result = compile_node(
        node->children[4], register_map, functions, parent, nonempty);

    if(!nonempty_result)
        return 0;

    builder->CreateBr(case_end);

    llvm::BasicBlock* nonempty_end = builder->GetInsertBlock();

    builder->SetInsertPoint(case_end);

    llvm::Type* result_type =
        generate_type(register_map, functions, node);

    if(!result_type)
        return 0;

    llvm::PHINode* phi = builder->CreatePHI(result_type, 2, "phi");

    phi->addIncoming(empty_result, empty_end);
    phi->addIncoming(nonempty_result, nonempty_end);

    return phi;
}

llvm::Value* Compiler::compile_node(
        cfl_typed_node* node,
        argument_register_map register_map,
        function_map functions,
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

        function_map::reverse_iterator function_itt = functions.rbegin();
        function_map::reverse_iterator function_end = functions.rend();

        for( ; function_itt != function_end; ++function_itt)
            if(!strcmp(function_itt->first, (char*) node->data))
            {
                function_map_result result = function_itt->second;

                if(result.struct_type == 0)
                    return builder->CreateCall(result.function_def, "constant_gen");
                else
                    return populate_function_struct(
                        register_map, result.node, result.array_type,
                        result.struct_type, result.application_def,
                        result.function_def, parent, entry_block);
            }

        if(!strcmp((char*) node->data, "random"))
            return create_random_function_struct(parent, entry_block);
    }
    else if(node->node_type == CFL_NODE_BOOL)
        return compile_node_bool(node);
    else if(node->node_type == CFL_NODE_INTEGER)
        return compile_node_integer(node);
    else if(node->node_type == CFL_NODE_CHAR)
        return compile_node_char(node);
    else if(node->node_type == CFL_NODE_FUNCTION)
        return compile_node_function(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_LIST)
        return compile_node_list(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_TUPLE)
        return compile_node_tuple(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_AND)
        return compile_node_and(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_OR)
        return compile_node_or(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_NOT)
        return compile_node_not(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_ADD)
        return compile_node_add(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_MULTIPLY)
        return compile_node_multiply(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_DIVIDE)
        return compile_node_divide(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_EQUAL)
        return compile_node_equal(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_LESS)
        return compile_node_less(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_APPLICATION)
        return compile_node_application(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_IF)
        return compile_node_if(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_LET_REC)
        return compile_node_let_rec(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_PUSH)
        return compile_node_push(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_CONCATENATE)
        return compile_node_concatenate(
            node, register_map, functions, parent, entry_block);
    else if(node->node_type == CFL_NODE_CASE)
        return compile_node_case(
            node, register_map, functions, parent, entry_block);

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

bool Compiler::compile_definitions(
        cfl_typed_definition_list* definitions,
        function_map& functions)
{
    while(definitions)
    {
        cfl_typed_node* definition = definitions->definition;

        if(definition->node_type == CFL_NODE_FUNCTION)
        {
            argument_type_map type_map;

            llvm::Type* argument_type =
                generate_type_inner(type_map, functions, definition->children[0]);

            if(!argument_type)
                return 0;

            argument_type_mapping mapping(definition->children[0], argument_type);
            type_map.push_back(mapping);

            llvm::FunctionType* application_type;
            llvm::FunctionType* function_type;
            llvm::ArrayType* array_type;
            llvm::StructType* struct_type;

            if(!generate_function_struct_types(
                    definition->children[0], definition->children[1], type_map,
                    functions, &application_type, &function_type, &array_type,
                    &struct_type))
                return 0;

            std::stringstream new_name;
            new_name << "__D__" << definitions->name;

            llvm::Function* function_def = llvm::Function::Create(
                function_type, llvm::Function::ExternalLinkage,
                new_name.str(), top_module);

            llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(
                global_context, "function_entry", function_def);

            builder->SetInsertPoint(function_entry);

            argument_register_map new_register_map;

            argument_type_map::iterator argument_type_itt = type_map.begin();
            argument_type_map::iterator argument_type_end = type_map.end();
            llvm::Function::arg_iterator arg_itt = function_def->arg_begin();

            for( ; argument_type_itt != argument_type_end; ++argument_type_itt)
            {
                argument_register_mapping new_mapping(
                    argument_type_itt->first, arg_itt++);
                new_register_map.push_back(new_mapping);
            }

            llvm::Value* result = compile_node(
                definition->children[1], new_register_map, functions,
                function_def, function_entry);

            if(!result)
                return 0;

            builder->CreateRet(result);

            llvm::Function* application_def = create_application_function(
                new_name.str(), array_type, struct_type,
                application_type, function_type, function_def);

            function_map_result function_result;

            function_result.node = definition;
            function_result.array_type = array_type;
            function_result.struct_type = struct_type;
            function_result.application_def = application_def;
            function_result.function_def = function_def;

            function_mapping f_mapping(definitions->name, function_result);
            functions.push_back(f_mapping);
        }
        else
        {
            argument_type_map type_map;

            llvm::Type* result_type =
                generate_type_inner(type_map, functions, definition);

            if(!result_type)
                return 0;

            llvm::FunctionType* function_type =
                llvm::FunctionType::get(result_type, false);

            std::stringstream new_name;
            new_name << "__D__" << definitions->name;

            llvm::Function* function_def = llvm::Function::Create(
                function_type, llvm::Function::ExternalLinkage,
                new_name.str(), top_module);

            llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(
                global_context, "function_entry", function_def);

            builder->SetInsertPoint(function_entry);

            argument_register_map new_register_map;

            argument_type_map::iterator argument_type_itt = type_map.begin();
            argument_type_map::iterator argument_type_end = type_map.end();
            llvm::Function::arg_iterator arg_itt = function_def->arg_begin();

            for( ; argument_type_itt != argument_type_end; ++argument_type_itt)
            {
                argument_register_mapping new_mapping(
                    argument_type_itt->first, arg_itt++);
                new_register_map.push_back(new_mapping);
            }

            llvm::Value* result = compile_node(
                definition, new_register_map, functions,
                function_def, function_entry);

            if(!result)
                return 0;

            builder->CreateRet(result);

            function_map_result function_result;

            function_result.node = definition;
            function_result.struct_type = 0;
            function_result.function_def = function_def;

            function_mapping f_mapping(definitions->name, function_result);
            functions.push_back(f_mapping);
        }

        definitions = definitions->next;
    }

    return true;
}

bool Compiler::compile_program(cfl_typed_program* program)
{
    setup_global_defs();

    function_map functions;

    if(!compile_definitions(program->definitions, functions))
        return false;

    llvm::FunctionType* main_type = llvm::FunctionType::get(
        builder->getInt32Ty(), false);

    llvm::Function* main_def = llvm::Function::Create(
        main_type, llvm::Function::ExternalLinkage, "main", top_module);

    llvm::BasicBlock* main_entry = llvm::BasicBlock::Create(
        global_context, "main_entry", main_def);

    builder->SetInsertPoint(main_entry);

    argument_register_map register_map;

    llvm::Value* result = compile_node(
        program->main, register_map, functions, main_def, main_entry);

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
    {
        std::cerr << "ERROR: Could not compile "
                  << filename_head << ".cfl" << std::endl;

        return false;
    }

    top_module->dump();

    delete builder;
    delete top_module;

    return true;
}

} // end namespace Cfl
