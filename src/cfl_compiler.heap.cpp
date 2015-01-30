#include "cfl_compiler.h"

namespace Cfl {

llvm::Value* Compiler::call_malloc(llvm::Type* type, llvm::Function* parent)
{
    std::vector<llvm::Type*> args;

#ifdef ARCH_32
    args.push_back(builder->getInt32Ty());
#else
    args.push_back(builder->getInt64Ty());
#endif

    llvm::ArrayRef<llvm::Type*> args_ref(args);

    llvm::FunctionType* malloc_type =
        llvm::FunctionType::get(builder->getInt8PtrTy(), args_ref, false);

    llvm::Value* malloc_def =
        top_module->getOrInsertFunction("malloc", malloc_type);

    llvm::DataLayout* data_layout = new llvm::DataLayout(top_module);

    uint64_t type_size = data_layout->getTypeAllocSize(type);

#ifdef ARCH_32
    llvm::Value* space =
        builder->CreateCall(malloc_def, builder->getInt32(type_size), "space");
#else
    llvm::Value* space =
        builder->CreateCall(malloc_def, builder->getInt32(type_size), "space");
#endif

    llvm::Value* is_null = builder->CreateIsNull(space, "is_null");

    llvm::BasicBlock* malloc_error = llvm::BasicBlock::Create(
        global_context, "__malloc_error", parent);
    llvm::BasicBlock* malloc_end = llvm::BasicBlock::Create(
        global_context, "__malloc_end", parent);

    builder->CreateCondBr(is_null, malloc_error, malloc_end);

    builder->SetInsertPoint(malloc_error);

    std::vector<llvm::Type*> exit_args;
    exit_args.push_back(builder->getInt32Ty());
    llvm::ArrayRef<llvm::Type*> exit_args_ref(exit_args);

    llvm::FunctionType* exit_type =
        llvm::FunctionType::get(builder->getVoidTy(), exit_args_ref, false);

    static llvm::Value* memory_error_string = builder->CreateGlobalStringPtr(
        "MEMORY ERROR: Could not allocate memory on the heap.");

    builder->CreateCall(global_puts, memory_error_string);

    llvm::Value* exit_def =
        top_module->getOrInsertFunction("exit", exit_type);

    builder->CreateCall(exit_def, builder->getInt32(2));

    builder->CreateUnreachable();

    builder->SetInsertPoint(malloc_end);

    return space;
}

void Compiler::call_free(llvm::Value* pointer)
{
    std::vector<llvm::Type*> args;
    args.push_back(builder->getInt8PtrTy());
    llvm::ArrayRef<llvm::Type*> args_ref(args);

    llvm::FunctionType* free_type =
        llvm::FunctionType::get(builder->getVoidTy(), args_ref, false);

    llvm::Value* free_def =
        top_module->getOrInsertFunction("free", free_type);

    builder->CreateCall(free_def, pointer);
}

} // end namespace Cfl
