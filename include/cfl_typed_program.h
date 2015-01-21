#ifndef _CFL_PROGRAM_LLVM_H_
#define _CFL_PROGRAM_LLVM_H_

#include "cfl_ast.h"
#include "cfl_type.types.h"

typedef struct cfl_typed_node_t {
    cfl_node_type node_type;
    cfl_type* resulting_type;
    unsigned int number_of_children;
    void* data;
    struct cfl_typed_node_t** children;
} cfl_typed_node;

cfl_typed_node* cfl_create_typed_node(cfl_node_type node_type,
                                      cfl_type* resulting_type,
                                      unsigned int number_of_children,
                                      void* data,
                                      cfl_typed_node** children);

void cfl_free_typed_node(cfl_typed_node* node);

typedef struct cfl_typed_definition_list_t {
    char* name;
    cfl_typed_node* definition;
    struct cfl_typed_definition_list_t* next;
} cfl_typed_definition_list;

void cfl_free_typed_definition_list(cfl_typed_definition_list* list);

typedef struct {
    cfl_typed_definition_list* definitions;
    cfl_typed_node* main;
} cfl_typed_program;

void cfl_free_typed_program(cfl_typed_program* program);
void cfl_print_typed_program(cfl_typed_program* program);

#endif
