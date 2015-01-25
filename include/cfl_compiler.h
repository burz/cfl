#ifndef _CFL_COMPILER_H_
#define _CFL_COMPILER_H_

extern "C" {
#include "cfl_typed_program.h"
}

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include <string>
#include <vector>

class CflCompiler
{
  private:
    llvm::LLVMContext& global_context;
    llvm::IRBuilder<>* builder;
    llvm::Module* top_module;
    llvm::Constant* global_puts;
    llvm::Constant* global_printf;
    llvm::Function* print_def;

    llvm::FunctionType* generate_function_type(unsigned int* number_of_args,
                                               cfl_type* type);

    bool generate_function_struct_types(cfl_type* type,
                                        llvm::FunctionType** function_type,
                                        llvm::StructType** struct_type,
                                        llvm::PointerType** struct_pointer_type);

    void generate_list_struct_types(llvm::StructType** struct_type,
                                    llvm::PointerType** struct_pointer_type);

    llvm::Type* generate_type(cfl_type* type);

    llvm::Value* compile_node_bool(cfl_typed_node* node);
    llvm::Value* compile_node_integer(cfl_typed_node* node);
    llvm::Value* compile_node_char(cfl_typed_node* node);

    typedef std::pair<cfl_typed_node*, llvm::Value*> argument_register_mapping;
    typedef std::vector<std::pair<cfl_typed_node*, llvm::Value*> > argument_register_map;
    typedef llvm::Value* node_compiler(cfl_typed_node* node,
                                       argument_register_map register_map,
                                       llvm::Function* parent,
                                       llvm::BasicBlock* entry_block);

    node_compiler compile_node_function;

    node_compiler compile_node_list;
    node_compiler compile_node_tuple;

    node_compiler compile_node_and;
    node_compiler compile_node_or;
    node_compiler compile_node_not;
    node_compiler compile_node_add;
    node_compiler compile_node_multiply;
    node_compiler compile_node_divide;
    node_compiler compile_node_equal;
    node_compiler compile_node_less;

    node_compiler compile_node_if;

    node_compiler compile_node;

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
