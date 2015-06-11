#include "cfl_compiler.h"

namespace Cfl {

llvm::StructType* Compiler::generate_random_function_struct_type(void)
{
    std::vector<llvm::Type*> cfl_rand_args;
    cfl_rand_args.push_back(builder->getInt32Ty());
    llvm::ArrayRef<llvm::Type*> cfl_rand_args_ref(cfl_rand_args);

    llvm::FunctionType* cfl_rand_type =
        llvm::FunctionType::get(builder->getInt32Ty(), cfl_rand_args_ref, false);

    llvm::ArrayType* array_type =
        llvm::ArrayType::get(builder->getInt8PtrTy(), 0);

    std::vector<llvm::Type*> members;
    members.push_back(cfl_rand_type);
    members.push_back(array_type->getPointerTo());
    llvm::ArrayRef<llvm::Type*> members_ref(members);

    return llvm::StructType::get(global_context, members_ref);
}

llvm::Value* Compiler::create_random_function_struct(
        llvm::Function* parent,
        llvm::BasicBlock* entry_block)
{
    llvm::Function* cfl_rand_def =
        llvm::cast<llvm::Function>(top_module->getFunction("__cfl_rand"));

    llvm::Function* cfl_rand_application_def =
        llvm::cast<llvm::Function>(top_module->getFunction("__cfl_rand_application"));

    if(!cfl_rand_def)
    {
        std::vector<llvm::Type*> time_args;
        time_args.push_back(builder->getInt32Ty());
        llvm::ArrayRef<llvm::Type*> time_args_ref(time_args);

        llvm::FunctionType* time_type = 
            llvm::FunctionType::get(builder->getInt32Ty(), time_args_ref, false);

        llvm::Constant* time_def = top_module->getOrInsertFunction("time", time_type);

        std::vector<llvm::Type*> srand_args;
        srand_args.push_back(builder->getInt32Ty());
        llvm::ArrayRef<llvm::Type*> srand_args_ref(srand_args);

        llvm::FunctionType* srand_type = 
            llvm::FunctionType::get(builder->getVoidTy(), srand_args_ref, false);

        llvm::Constant* srand_def = top_module->getOrInsertFunction("srand", srand_type);

        llvm::BasicBlock* main_block =
            top_module->getFunction("main")->begin()++;

        llvm::Instruction* first_instruction = main_block->begin()++;

        llvm::CallInst* time_call =
            llvm::CallInst::Create(time_def, builder->getInt32(0));

        main_block->getInstList().insert(first_instruction, time_call);

        llvm::CallInst* srand_call =
            llvm::CallInst::Create(srand_def, time_call);

        main_block->getInstList().insert(first_instruction, srand_call);

        std::vector<llvm::Type*> rand_args;
        llvm::ArrayRef<llvm::Type*> rand_args_ref(rand_args);

        llvm::FunctionType* rand_type = 
            llvm::FunctionType::get(builder->getInt32Ty(), rand_args_ref, false);

        llvm::Constant* rand_def = top_module->getOrInsertFunction("rand", rand_type);

        std::vector<llvm::Type*> cfl_rand_args;
        cfl_rand_args.push_back(builder->getInt32Ty());
        llvm::ArrayRef<llvm::Type*> cfl_rand_args_ref(cfl_rand_args);

        llvm::FunctionType* cfl_rand_type = 
            llvm::FunctionType::get(builder->getInt32Ty(), cfl_rand_args_ref, false);

        cfl_rand_def = llvm::Function::Create(
            cfl_rand_type, llvm::Function::ExternalLinkage, "__cfl_rand", top_module);

        llvm::BasicBlock* function_entry = llvm::BasicBlock::Create(
            global_context, "function_entry", cfl_rand_def);

        builder->SetInsertPoint(function_entry);

        llvm::Value* random_number = builder->CreateCall(
            rand_def, "random_number");

        llvm::Value* successor = builder->CreateAdd(
            cfl_rand_def->arg_begin()++, builder->getInt32(1), "successor");

        llvm::Value* result =
            builder->CreateSRem(random_number, successor, "result");

        builder->CreateRet(result);

        std::vector<llvm::Type*> cfl_rand_application_args;
        cfl_rand_application_args.push_back(builder->getInt8PtrTy());
        cfl_rand_application_args.push_back(builder->getInt8PtrTy());
        cfl_rand_application_args.push_back(builder->getInt32Ty());
        llvm::ArrayRef<llvm::Type*> cfl_rand_application_args_ref(cfl_rand_application_args);

        llvm::FunctionType* cfl_rand_application_type =
            llvm::FunctionType::get(builder->getInt32Ty(), cfl_rand_application_args_ref, false);

        cfl_rand_application_def = llvm::Function::Create(
            cfl_rand_application_type, llvm::Function::ExternalLinkage,
            "__cfl_rand_application", top_module);

        llvm::BasicBlock* application_function_entry = llvm::BasicBlock::Create(
            global_context, "function_entry", cfl_rand_application_def);

        builder->SetInsertPoint(application_function_entry);

        llvm::Function::arg_iterator arg_itt = cfl_rand_application_def->arg_begin();

        llvm::Value* function_pointer = arg_itt++;

        llvm::Value* function = builder->CreatePointerCast(
            function_pointer, cfl_rand_type->getPointerTo(), "function");

        arg_itt++;

        llvm::Value* argument = arg_itt++;

        llvm::Value* applied_result = builder->CreateCall(function, argument, "applied_result");

        builder->CreateRet(applied_result);

        builder->SetInsertPoint(entry_block);
    }

    std::vector<llvm::Type*> members;
    members.push_back(cfl_rand_application_def->getType()->getPointerTo());
    members.push_back(builder->getInt8PtrTy());
    members.push_back(builder->getInt8PtrTy());
    llvm::ArrayRef<llvm::Type*> members_ref(members);

    llvm::StructType* struct_type = llvm::StructType::get(global_context, members_ref);

    std::vector<llvm::Constant*> function_values;
    function_values.push_back(cfl_rand_application_def);
    function_values.push_back(
        llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    function_values.push_back(
        llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
    llvm::ArrayRef<llvm::Constant*> function_values_ref(function_values);

    llvm::Constant* initial_struct =
        llvm::ConstantStruct::get(struct_type, function_values_ref);

    llvm::Value* cfl_rand_pointer =
        builder->CreatePointerCast(cfl_rand_def, builder->getInt8PtrTy());

    return builder->CreateInsertValue(initial_struct, cfl_rand_pointer, 1, "function_struct");
}

} // end namespace Cfl
