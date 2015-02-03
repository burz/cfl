#include "cfl_compiler.h"

namespace Cfl {

static cfl_typed_node* find_leaf_node(cfl_typed_node* node)
{
    for(;;)
    {
        if(node->node_type == CFL_NODE_IF ||
           node->node_type == CFL_NODE_CASE)
        {
            node = node->children[1];

            continue;
        }

        return node;
    }
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

    llvm::Type* input_type =
        generate_type_inner(saved_argument_types, node->children[0]);

    if(!input_type)
        return 0;

    args.push_back(input_type);

    argument_type_mapping mapping(node->children[0], input_type);

    new_argument_types.push_back(mapping);

    llvm::ArrayRef<llvm::Type*> args_ref(args);

    cfl_typed_node* expression = find_leaf_node(node->children[1]);

    llvm::Type* return_type =
        generate_type_inner(new_argument_types, expression);

    *function_type =
        llvm::FunctionType::get(return_type, args_ref, false);

    if(!*function_type)
        return 0;

    llvm::PointerType* function_pointer_type =
        llvm::PointerType::getUnqual(*function_type);

    llvm::ArrayType* array_type =
        llvm::ArrayType::get(builder->getInt8PtrTy(), args.size() - 1);

    std::vector<llvm::Type*> members;
    members.push_back(function_pointer_type);
    members.push_back(array_type->getPointerTo());
    llvm::ArrayRef<llvm::Type*> members_ref(members);

    *struct_type = llvm::StructType::get(global_context, members_ref);

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

        if(!generate_function_struct_types(
                node, type_map, &function_type, &struct_type))
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

} // end namespace Cfl
