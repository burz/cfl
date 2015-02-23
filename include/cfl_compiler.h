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

namespace Cfl {

class Compiler
{
  private:
    llvm::LLVMContext& global_context;
    llvm::IRBuilder<>* builder;
    llvm::Module* top_module;
    llvm::Constant* global_puts;
    llvm::Constant* global_printf;
    llvm::Function* print_def;

    typedef std::pair<cfl_typed_node*, llvm::Type*> argument_type_mapping;
    typedef std::vector<argument_type_mapping> argument_type_map;

    typedef std::pair<cfl_typed_node*, llvm::Value*> argument_register_mapping;
    typedef std::vector<argument_register_mapping> argument_register_map;

    typedef struct {
        cfl_typed_node* node;
        llvm::ArrayType* array_type;
        llvm::StructType* struct_type;
        llvm::Constant* application_def;
        llvm::Constant* function_def;
    } function_map_result;

    typedef std::pair<char*, function_map_result> function_mapping;
    typedef std::vector<function_mapping> function_map;

    typedef llvm::Value* node_compiler(cfl_typed_node* node,
                                       argument_register_map register_map,
                                       function_map functions,
                                       llvm::Function* parent,
                                       llvm::BasicBlock* entry_block);

    bool generate_function_struct_types(cfl_typed_node* argument,
                                        cfl_typed_node* expression,
                                        argument_type_map type_map,
                                        function_map functions,
                                        llvm::FunctionType** application_type,
                                        llvm::FunctionType** function_type,
                                        llvm::ArrayType** array_type,
                                        llvm::StructType** struct_type);

    void generate_list_struct_types(llvm::StructType** struct_type,
                                    llvm::PointerType** struct_pointer_type);

    llvm::Type* generate_type_inner(argument_type_map type_map,
                                    function_map functions,
                                    cfl_typed_node* node);

    llvm::Type* generate_type(argument_register_map register_map,
                              function_map functions,
                              cfl_typed_node* node);

    llvm::Value* compile_node_bool(cfl_typed_node* node);
    llvm::Value* compile_node_integer(cfl_typed_node* node);
    llvm::Value* compile_node_char(cfl_typed_node* node);

    llvm::Value* call_malloc(llvm::Type* type,
                             llvm::Function* parent,
                             llvm::BasicBlock* entry_block);
    void call_free(llvm::Value* pointer);

    llvm::StructType* generate_random_function_struct_type(void);
    llvm::Value* create_random_function_struct(llvm::Function* parent,
                                               llvm::BasicBlock* entry_block);

    llvm::Function* create_application_function(std::string name,
                                                llvm::ArrayType* array_type,
                                                llvm::StructType* struct_type,
                                                llvm::FunctionType* application_type,
                                                llvm::FunctionType* function_type,
                                                llvm::Constant* function_def);

    llvm::Value* populate_function_struct(argument_register_map register_map,
                                          cfl_typed_node* node,
                                          llvm::ArrayType* array_type,
                                          llvm::StructType* struct_type,
                                          llvm::Constant* application_def,
                                          llvm::Constant* function_def,
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
    node_compiler compile_node_application;
    node_compiler compile_node_if;
    node_compiler compile_node_let_rec;
    node_compiler compile_node_push;
    node_compiler compile_node_concatenate;
    node_compiler compile_node_case;
    node_compiler compile_node;

    void setup_global_defs(void);

    llvm::Value* extract_value_from_pointer(llvm::Value* pointer, cfl_type* type);

    void generate_print_function(cfl_type* result_type,
                                 llvm::Value* result,
                                 llvm::BasicBlock* block,
                                 bool in_string = false);

    bool compile_definitions(cfl_typed_definition_list* definitions,
                             function_map& functions);

    bool compile_program(cfl_typed_program* program);
  public:
    Compiler(void);

    bool compile(cfl_typed_program* program, std::string& filename_head);
};

} // end namespace Cfl

#endif
