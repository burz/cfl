#include "cfl_compiler.h"

#include <iostream>
#include <vector>
#include <sstream>

CflCompiler::CflCompiler(void)
    : global_context(llvm::getGlobalContext())
{
}

void CflCompiler::generate_list_struct_types(
        cfl_type* type,
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

        generate_list_struct_types((cfl_type*) type->input, &struct_type, &pointer_type);

        return pointer_type;
    }
    else if(type->type == CFL_TYPE_TUPLE)
        return llvm::ArrayType::get(builder->getInt8PtrTy(), type->id);
    else if(type->type == CFL_TYPE_ARROW)
        return builder->getInt8PtrTy();

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

llvm::Value* CflCompiler::compile_node_list(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::StructType* list_type;
    llvm::PointerType* list_pointer_type;

    generate_list_struct_types(node->resulting_type, &list_type, &list_pointer_type);

    if(!node->data)
        return llvm::ConstantPointerNull::get(list_pointer_type);

    llvm::Type* element_type = generate_type((cfl_type*) node->resulting_type->input);

    cfl_typed_node_list* pos = (cfl_typed_node_list*) node->data;

    llvm::Value* head = 0;
    llvm::Value* last_node = 0;

    while(pos)
    {
        llvm::Value* element = compile_node(pos->node, parent);

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

llvm::Value* CflCompiler::compile_node_tuple(cfl_typed_node* node, llvm::Function* parent)
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

llvm::Value* CflCompiler::compile_node_and(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateAnd(left, right, "and");
}

llvm::Value* CflCompiler::compile_node_or(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateOr(left, right, "or");
}

llvm::Value* CflCompiler::compile_node_not(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* child = compile_node(node->children[0], parent);

    return builder->CreateNot(child, "not");
}

llvm::Value* CflCompiler::compile_node_add(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateAdd(left, right, "add");
}

llvm::Value* CflCompiler::compile_node_multiply(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateMul(left, right, "multiply");
}

llvm::Value* CflCompiler::compile_node_divide(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateSDiv(left, right, "divide");
}

llvm::Value* CflCompiler::compile_node_equal(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateICmpEQ(left, right, "equal");
}

llvm::Value* CflCompiler::compile_node_less(cfl_typed_node* node, llvm::Function* parent)
{
    llvm::Value* left = compile_node(node->children[0], parent);
    llvm::Value* right = compile_node(node->children[1], parent);

    return builder->CreateICmpSLT(left, right, "less");
}

llvm::Value* CflCompiler::compile_node_if(cfl_typed_node* node, llvm::Function* parent)
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

llvm::Value* CflCompiler::compile_node(cfl_typed_node* node, llvm::Function* parent)
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

void CflCompiler::setup_global_defs(void)
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

        generate_list_struct_types(type, &list_type, &list_pointer_type);

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

static std::string cfl_arrow_string_inner(cfl_type* type)
{
    if(type->type == CFL_TYPE_VARIABLE)
    {
        std::stringstream number;
        number << type->id;

        return "a" + number.str();
    }
    else if(type->type == CFL_TYPE_BOOL)
        return "Boolean";
    else if(type->type == CFL_TYPE_INTEGER)
        return "Integer";
    else if(type->type == CFL_TYPE_CHAR)
        return "Char";
    else if(type->type == CFL_TYPE_LIST)
        return "[" + cfl_arrow_string_inner((cfl_type*) type->input) + "]";
    else if(type->type == CFL_TYPE_TUPLE)
    {
        std::string result = "(";

        int i = 0;
        for( ; i < type->id; ++i)
        {
            result += cfl_arrow_string_inner(((cfl_type**) type->input)[i]);

            if(i < type->id - 1)
                result += ", ";

            return result + ")";
        }
    }
    else if(type->type == CFL_TYPE_ARROW)
        return cfl_arrow_string_inner((cfl_type*) type->input) + " -> " +
               cfl_arrow_string_inner((cfl_type*) type->output);

    return "";
}

static std::string cfl_arrow_string(cfl_type* type)
{
    return "function :: " + cfl_arrow_string_inner(type);
}

void CflCompiler::generate_print_function(
        cfl_type* result_type,
        llvm::Value* result,
        llvm::BasicBlock* block)
{
    static llvm::Value* comma = 0;

    if(!comma && (result_type->type == CFL_TYPE_LIST ||
                  result_type->type == CFL_TYPE_TUPLE))
        comma = builder->CreateGlobalStringPtr(", ");

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
    else if(result_type->type == CFL_TYPE_LIST)
    {
        static llvm::Value* open_bracket = builder->CreateGlobalStringPtr("[");
        static llvm::Value* close_bracket = builder->CreateGlobalStringPtr("]");
        llvm::StructType* list_type;
        llvm::PointerType* list_pointer_type;

        generate_list_struct_types(result_type, &list_type, &list_pointer_type);

        std::vector<llvm::Type*> print_args;
        print_args.push_back(list_pointer_type);
        llvm::ArrayRef<llvm::Type*> print_args_ref(print_args);

        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), print_args_ref, false);

        llvm::Function* new_print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_list", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_list_entry", new_print_def);

        builder->SetInsertPoint(print_entry);

        builder->CreateCall(global_printf, open_bracket);

        llvm::Value* argument = new_print_def->arg_begin()++;

        llvm::BasicBlock* print_loop = llvm::BasicBlock::Create(
            global_context, "__print_list_loop", new_print_def);
        llvm::BasicBlock* print_comma = llvm::BasicBlock::Create(
            global_context, "__print_comma", new_print_def);
        llvm::BasicBlock* print_end = llvm::BasicBlock::Create(
            global_context, "__print_list_end", new_print_def);

        llvm::Value* is_not_null = builder->CreateIsNotNull(argument, "is_not_null");

        llvm::AllocaInst* pos_pointer_space = builder->CreateAlloca(
            list_pointer_type, builder->getInt32(1), "pos_pointer_space");

        builder->CreateStore(argument, pos_pointer_space);

        builder->CreateCondBr(is_not_null, print_loop, print_end);

        builder->SetInsertPoint(print_loop);

        llvm::Value* pos = builder->CreateLoad(pos_pointer_space);

        llvm::Value* list_node = builder->CreateLoad(pos, "pos");

        llvm::Value* extracted = builder->CreateExtractValue(list_node, 0);

        llvm::Value* element =
            extract_value_from_pointer(extracted, (cfl_type*) result_type->input);

        generate_print_function((cfl_type*) result_type->input, element, print_loop);

        llvm::Value* new_pos = builder->CreateExtractValue(list_node, 1, "new_pos");

        builder->CreateStore(new_pos, pos_pointer_space);

        is_not_null = builder->CreateIsNotNull(new_pos, "is_not_null");

        builder->CreateCondBr(is_not_null, print_comma, print_end);

        builder->SetInsertPoint(print_comma);

        builder->CreateCall(global_printf, comma);

        builder->CreateBr(print_loop);

        builder->SetInsertPoint(print_end);

        print_def = new_print_def;

        builder->CreateCall(global_printf, close_bracket);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def, result);
    }
    else if(result_type->type == CFL_TYPE_TUPLE)
    {
        static llvm::Value* open_parentheses = builder->CreateGlobalStringPtr("(");
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
    else if(result_type->type == CFL_TYPE_ARROW)
    {
        std::string generated_function_string = cfl_arrow_string(result_type);

        llvm::FunctionType* print_type =
            llvm::FunctionType::get(builder->getVoidTy(), false);

        print_def = llvm::Function::Create(
            print_type, llvm::Function::ExternalLinkage, "__print_function", top_module);

        llvm::BasicBlock* print_entry = llvm::BasicBlock::Create(
            global_context, "__print_function", print_def);

        builder->SetInsertPoint(print_entry);

        llvm::Value* function_string =
            builder->CreateGlobalStringPtr(generated_function_string);

        builder->CreateCall(global_printf, function_string);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def);
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

        static llvm::Value* success_string = builder->CreateGlobalStringPtr("Success.");

        builder->CreateCall(global_printf, success_string);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def);
    }
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

bool CflCompiler::compile(cfl_typed_program* program, std::string& destination_file)
{
    builder = new llvm::IRBuilder<>(global_context);
    top_module = new llvm::Module(destination_file, global_context);

    if(!compile_program(program))
        return false;

    top_module->dump();

    delete builder;
// TODO:
//    delete top_module;

    return true;
}
