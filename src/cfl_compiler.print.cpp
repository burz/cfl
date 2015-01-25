#include "cfl_compiler.h"

#include <sstream>

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
        }

        return result + ")";
    }
    else if(type->type == CFL_TYPE_ARROW)
        return cfl_arrow_string_inner((cfl_type*) type->input) + " -> (" +
               cfl_arrow_string_inner((cfl_type*) type->output) + ")";

    return "";
}

static std::string cfl_arrow_string(cfl_type* type)
{
    return "function :: " + cfl_arrow_string_inner(type);
}

void CflCompiler::generate_print_function(
        cfl_type* result_type,
        llvm::Value* result,
        llvm::BasicBlock* block,
        bool in_string)
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

        llvm::Value* format_string;

        if(in_string)
            format_string = builder->CreateGlobalStringPtr("%c");
        else
            format_string = builder->CreateGlobalStringPtr("'%c'");

        builder->CreateCall2(global_printf, format_string, print_def->arg_begin()++);
        builder->CreateRetVoid();

        builder->SetInsertPoint(block);

        builder->CreateCall(print_def, result);
    }
    else if(result_type->type == CFL_TYPE_LIST)
    {
        static llvm::Function* print_string = 0;

        bool is_string = ((cfl_type*) result_type->input)->type == CFL_TYPE_CHAR;

        if(print_string && is_string)
        {
            builder->CreateCall(print_string, result);

            return;
        }

        llvm::StructType* list_type;
        llvm::PointerType* list_pointer_type;

        generate_list_struct_types(&list_type, &list_pointer_type);

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

        llvm::Value* quote = 0;

        if(is_string)
        {
            quote = builder->CreateGlobalStringPtr("\"");

            builder->CreateCall(global_printf, quote);
        }
        else
        {
            static llvm::Value* open_brackets = builder->CreateGlobalStringPtr("[");

            builder->CreateCall(global_printf, open_brackets);
        }

        llvm::Value* argument = new_print_def->arg_begin()++;

        llvm::BasicBlock* print_loop = llvm::BasicBlock::Create(
            global_context, "__print_list_loop", new_print_def);
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

        generate_print_function((cfl_type*) result_type->input, element, print_loop, true);

        llvm::Value* new_pos = builder->CreateExtractValue(list_node, 1, "new_pos");

        builder->CreateStore(new_pos, pos_pointer_space);

        is_not_null = builder->CreateIsNotNull(new_pos, "is_not_null");

        if(is_string)
            builder->CreateCondBr(is_not_null, print_loop, print_end);
        else
        {
            llvm::BasicBlock* print_comma = llvm::BasicBlock::Create(
                global_context, "__print_comma", new_print_def);

            builder->CreateCondBr(is_not_null, print_comma, print_end);

            builder->SetInsertPoint(print_comma);

            builder->CreateCall(global_printf, comma);

            builder->CreateBr(print_loop);
        }

        builder->SetInsertPoint(print_end);

        print_def = new_print_def;

        if(is_string)
            builder->CreateCall(global_printf, quote);
        else
        {
            static llvm::Value* close_brackets = builder->CreateGlobalStringPtr("]");

            builder->CreateCall(global_printf, close_brackets);
        }

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
}
