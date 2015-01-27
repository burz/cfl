#ifndef _CFL_TYPED_PROGRAM_H_
#define _CFL_TYPED_PROGRAM_H_

#include "cfl_typed_program.types.h"
#include "cfl_ast.h"
#include "cfl_type.types.h"

bool cfl_is_free_in_typed_node(char* name, cfl_typed_node* node);

cfl_typed_node* cfl_create_typed_node(cfl_node_type node_type,
                                      cfl_type* resulting_type,
                                      unsigned int number_of_children,
                                      void* data,
                                      cfl_typed_node** children);

void cfl_free_typed_node(cfl_typed_node* node);

void cfl_free_typed_node_list(cfl_typed_node_list* list);

void cfl_free_typed_definition_list(cfl_typed_definition_list* list);

void cfl_free_typed_program(cfl_typed_program* program);
void cfl_print_typed_program(cfl_typed_program* program);

#endif
