#ifndef _CFL_COMPILER_H_
#define _CFL_COMPILER_H_

extern "C" {
#include "cfl_typed_program.h"
}

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include <string>

class CflCompiler
{
  private:
    llvm::LLVMContext& global_context;
    llvm::IRBuilder<>* builder;
    llvm::Module* top_module;
    llvm::Value* cfl_error_division_by_zero_string;
    llvm::Constant* global_puts;
    llvm::Constant* global_printf;
    llvm::Function* print_def;

    void generate_list_struct_types(cfl_type* type,
                                    llvm::StructType** struct_type,
                                    llvm::PointerType** struct_pointer_type);

    llvm::Type* generate_type(cfl_type* type);

    llvm::Value* compile_node_bool(cfl_typed_node* node);
    llvm::Value* compile_node_integer(cfl_typed_node* node);
    llvm::Value* compile_node_char(cfl_typed_node* node);

    llvm::Value* compile_node_list(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_tuple(cfl_typed_node* node, llvm::Function* parent);

    llvm::Value* compile_node_and(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_or(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_not(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_add(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_multiply(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_divide(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_equal(cfl_typed_node* node, llvm::Function* parent);
    llvm::Value* compile_node_less(cfl_typed_node* node, llvm::Function* parent);

    llvm::Value* compile_node_if(cfl_typed_node* node, llvm::Function* parent);

    llvm::Value* compile_node(cfl_typed_node* node, llvm::Function* parent);

    void setup_global_defs(void);

    llvm::Value* extract_value_from_pointer(llvm::Value* pointer, cfl_type* type);

    void generate_print_function(cfl_type* result_type,
                                 llvm::Value* result,
                                 llvm::BasicBlock* block);

    bool compile_program(cfl_typed_program* program);
  public:
    CflCompiler(void);

    bool compile(cfl_typed_program* program, std::string& destination_file);
};

#endif
