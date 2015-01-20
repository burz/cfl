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
    llvm::Constant* global_puts;
    llvm::Constant* global_printf;
    llvm::Function* print_def;

    llvm::Value* compile_node_bool(cfl_node* node);
    llvm::Value* compile_node_integer(cfl_node* node);
    llvm::Value* compile_node_char(cfl_node* node);

    llvm::Value* compile_node_and(cfl_node* node, llvm::BasicBlock* block);
    llvm::Value* compile_node_or(cfl_node* node, llvm::BasicBlock* block);
    llvm::Value* compile_node_not(cfl_node* node, llvm::BasicBlock* block);
    llvm::Value* compile_node_add(cfl_node* node, llvm::BasicBlock* block);
    llvm::Value* compile_node_multiply(cfl_node* node, llvm::BasicBlock* block);
    llvm::Value* compile_node_divide(cfl_node* node, llvm::BasicBlock* block);
    llvm::Value* compile_node_equal(cfl_node* node, llvm::BasicBlock* block);
    llvm::Value* compile_node_less(cfl_node* node, llvm::BasicBlock* block);

    llvm::Value* compile_node(cfl_node* node, llvm::BasicBlock* block);

    void setup_global_defs(void);
    void generate_print_function(cfl_program* program);
    bool compile_program(cfl_program* program);
  public:
    cfl_Compiler(void);

    bool compile(cfl_program* program, std::string& destination_file);
};

#endif
