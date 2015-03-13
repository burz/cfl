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

bool Compiler::is_not_bound_in(
        cfl_typed_node* variable,
        cfl_typed_node* complex_variable)
{
    if(variable->node_type == CFL_NODE_VARIABLE)
    {
        if(complex_variable->node_type == CFL_NODE_VARIABLE)
        {
            if(*((char*) complex_variable->data) == '_')
                return true;

            return strcmp((char*) variable->data, (char*) complex_variable->data);
        }

        for(int i = 0; i < complex_variable->number_of_children; ++i)
            if(!strcmp((char*) variable->data, (char*) complex_variable->children[i]->data))
                return false;

        return true;
    }
    else
    {
        for(int i = 0; i < variable->number_of_children; ++i)
        {
            if(*((char*) variable->children[i]->data) == '_')
                    continue;

            if(complex_variable->node_type == CFL_NODE_VARIABLE)
            {
                if(*((char*) complex_variable->data) == '_')
                    continue;

                if(!strcmp((char*) variable->children[i]->data,
                           (char*) complex_variable->data))
                    return false;
            }
            else
                for(int j = 0; j < complex_variable->number_of_children; ++j)
                    if(!strcmp((char*) variable->children[i]->data,
                               (char*) complex_variable->children[j]->data))
                        return false;
        }

        return true;
    }
}

bool Compiler::generate_function_struct_types(
        cfl_typed_node* argument,
        cfl_typed_node* expression,
        argument_type_map saved_argument_types,
        function_map functions,
        llvm::FunctionType** application_type,
        llvm::FunctionType** function_type,
        llvm::ArrayType** array_type,
        llvm::StructType** struct_type)
{
    std::vector<llvm::Type*> args;
    argument_type_map new_argument_types;

    argument_type_map::iterator itt = saved_argument_types.begin();
    argument_type_map::iterator end = saved_argument_types.end();

    for( ; itt != end; ++itt)
        if(is_not_bound_in(itt->first, argument) &&
           cfl_is_free_in_typed_node((char*) itt->first->data, expression))
        {
            args.push_back(itt->second); 
            new_argument_types.push_back(*itt);
        }

    llvm::Type* input_type =
        generate_type_inner(saved_argument_types, functions, argument);

    if(!input_type)
        return 0;

    args.push_back(input_type);

    argument_type_mapping mapping(argument, input_type);

    new_argument_types.push_back(mapping);

    llvm::ArrayRef<llvm::Type*> args_ref(args);

    cfl_typed_node* resulting_expression = find_leaf_node(expression);

    llvm::Type* return_type =
        generate_type_inner(new_argument_types, functions, resulting_expression);

    if(!return_type)
        return 0;

    std::vector<llvm::Type*> application_args;
    application_args.push_back(builder->getInt8PtrTy());
    application_args.push_back(builder->getInt8PtrTy());
    application_args.push_back(input_type);
    llvm::ArrayRef<llvm::Type*> application_args_ref(application_args);

    *application_type =
        llvm::FunctionType::get(return_type, application_args_ref, false);

    *function_type =
        llvm::FunctionType::get(return_type, args_ref, false);

    if(!*function_type)
        return 0;

    *array_type =
        llvm::ArrayType::get(builder->getInt8PtrTy(), args.size() - 1);

    std::vector<llvm::Type*> members;
    members.push_back((*application_type)->getPointerTo());
    members.push_back(builder->getInt8PtrTy());
    members.push_back(builder->getInt8PtrTy());
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
        function_map functions,
        cfl_typed_node* node)
{
    if(node->resulting_type->type == CFL_TYPE_VARIABLE)
        return builder->getInt8PtrTy();
    else if(node->resulting_type->type == CFL_TYPE_BOOL)
        return builder->getInt1Ty();
    else if(node->resulting_type->type == CFL_TYPE_INTEGER)
        return builder->getInt32Ty();
    else if(node->resulting_type->type == CFL_TYPE_CHAR)
        return builder->getInt8Ty();
    else if(node->node_type == CFL_NODE_VARIABLE)
    {
        char* name = (char*) node->data;

        argument_type_map::iterator itt = type_map.begin();
        argument_type_map::iterator end = type_map.end();

        for( ; itt != end; ++itt)
            if(itt->first->node_type == CFL_NODE_VARIABLE &&
               !strcmp((char*) itt->first->data, name))
                return itt->second;

        function_map::reverse_iterator function_itt = functions.rbegin();
        function_map::reverse_iterator function_end = functions.rend();

        for( ; function_itt != function_end; ++function_itt)
            if(!strcmp(function_itt->first, name))
                return function_itt->second.struct_type;

        if(!strcmp(name, "random"))
            return generate_random_function_struct_type();
    }
    else if(node->node_type == CFL_NODE_IF)
        return generate_type_inner(type_map, functions, node->children[1]);
    else if(node->node_type == CFL_NODE_CASE)
        return generate_type_inner(type_map, functions, node->children[1]);

    if(node->resulting_type->type == CFL_TYPE_LIST)
    {
        llvm::StructType* struct_type;
        llvm::PointerType* pointer_type;

        generate_list_struct_types(&struct_type, &pointer_type);

        return pointer_type;
    }
    else if(node->resulting_type->type == CFL_TYPE_TUPLE)
        return llvm::ArrayType::get(builder->getInt8PtrTy(),
                                    node->resulting_type->id);
    else if(node->resulting_type->type == CFL_TYPE_ARROW)
    {
        if(node->node_type == CFL_NODE_LET_REC)
        {
            argument_type_map new_type_map;

            argument_type_map::iterator itt = type_map.begin();
            argument_type_map::iterator end = type_map.end();

            for( ; itt != end; ++itt)
            {
                char* argument = (char*) itt->first->data;

                if(strcmp(argument, (char*) node->children[0]->data) &&
                   strcmp(argument, (char*) node->children[1]->data) &&
                   cfl_is_free_in_typed_node(argument, node->children[2]))
                    new_type_map.push_back(*itt);
            }

            llvm::Type* argument_type =
                generate_type_inner(type_map, functions, node->children[1]);

            if(!argument_type)
                return 0;

            argument_type_mapping mapping(node->children[1], argument_type);
            new_type_map.push_back(mapping);

            llvm::FunctionType* application_type;
            llvm::FunctionType* function_type;
            llvm::ArrayType* array_type;
            function_map_result map_result;

            if(!generate_function_struct_types(
                    node->children[1], node->children[2], new_type_map,
                    functions, &application_type, &function_type, &array_type,
                    &map_result.struct_type))
                return 0;

            function_mapping f_mapping((char*) node->children[0]->data, map_result);
            functions.push_back(f_mapping);

            return generate_type_inner(type_map, functions, node->children[3]);
        }
        else
        {
            llvm::FunctionType* application_type;
            llvm::FunctionType* function_type;
            llvm::ArrayType* array_type;
            llvm::StructType* struct_type;

            if(!generate_function_struct_types(
                    node->children[0], node->children[1], type_map,
                    functions, &application_type, &function_type,
                    &array_type, &struct_type))
                return 0;

            return struct_type;
        }
    }

    return 0;
}

llvm::Type* Compiler::generate_type(
        argument_register_map register_map,
        function_map functions,
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

    return generate_type_inner(type_map, functions, node);
}

} // end namespace Cfl
