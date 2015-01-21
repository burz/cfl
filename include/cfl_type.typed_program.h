#ifndef _CFL_TYPE_TYPED_PROGRAM_H_
#define _CFL_TYPE_TYPED_PROGRAM_H_

#include "cfl_typed_program.h"
#include "cfl_program.h"

cfl_typed_node* cfl_generate_typed_node(cfl_type_equations* equations,
                                        cfl_type_hypothesis_chain* hypothesis_head,
                                        cfl_typed_definition_list* definitions,
                                        cfl_node* node);

bool cfl_simplify_typed_node(cfl_type_equations* equations, cfl_typed_node* node);

cfl_typed_definition_list* cfl_generate_definition_types(cfl_type_hypothesis_chain* hypothesis_head,
                                                         cfl_definition_list* definitions,
                                                         unsigned int equation_hash_table_length);

cfl_typed_program* cfl_generate_typed_program(cfl_program* program,
                                              unsigned int equation_hash_table_length);

#endif
