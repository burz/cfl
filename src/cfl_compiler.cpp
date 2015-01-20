#include "cfl_compiler.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

bool cfl_compile(cfl_program* program, std::string& destination_file)
{
    llvm::LLVMContext& global_context = llvm::getGlobalContext();
    llvm::Module* top_module = new llvm::Module("top", global_context);
    llvm::IRBuilder<> builder(global_context);

    top_module->dump();

    return true;
}
