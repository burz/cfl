#ifndef _CFL_COMPILER_H_
#define _CFL_COMPILER_H_

extern "C" {
#include "cfl_program.h"
}

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include <string>

class cfl_Compiler
{
  private:
    llvm::LLVMContext& global_context;
    llvm::IRBuilder<>* builder;
    llvm::Module* top_module;
    llvm::Value* cfl_error_division_by_zero_string;

    llvm::Value* compile_node_bool(cfl_node* node);

    llvm::Value* compile_node_and(cfl_node* node, llvm::BasicBlock* block);

    llvm::Value* compile_node(cfl_node* node, llvm::BasicBlock* block);

    bool compile_program(cfl_program* program);
  public:
    cfl_Compiler(void);

    bool compile(cfl_program* program, std::string& destination_file);
};

#endif
