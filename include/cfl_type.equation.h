#ifndef _CFL_TYPE_EQUATION_H_
#define _CFL_TYPE_EQUATION_H_

#include "cfl_ast.h"
#include "cfl_type.types.h"

unsigned long long cfl_hash_type(cfl_type* type);

bool cfl_initialize_type_equations(cfl_type_equations* equations,
                                   unsigned int equation_hash_table_length);
cfl_type_equations* cfl_copy_type_equations(cfl_type_equations* equations);
void cfl_delete_type_equations(cfl_type_equations* equations);

int cfl_add_type_equation(cfl_type_equations* equations,
                          cfl_type* left,
                          cfl_type* right);

int cfl_add_type_equations(cfl_type_equations* equations,
                           cfl_type* left,
                           cfl_type* right);

int cfl_add_type_equations_from_copies(cfl_type_equations* equations,
                                       cfl_type* left,
                                       cfl_type* right);

bool cfl_close_type_equations(cfl_type_equations* equations);

bool cfl_are_type_equations_consistent(cfl_type_equations* equations);

bool cfl_simplify_type(cfl_type_equations* equations, cfl_type* node);

#endif
