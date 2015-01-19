#ifndef _CFL_EVAL_H_
#define _CFL_EVAL_H_

#include "cfl_ast.h"
#include "cfl_program.h"

void cfl_initialize_eval(void);

bool cfl_substitute(cfl_node* target, char* variable, cfl_node* value);
bool cfl_complex_substitute(cfl_node* target, cfl_node* variable, cfl_node* value);

int cfl_evaluate_global_function_random(int value);

bool cfl_evaluate(cfl_node* node, cfl_definition_list* definitions);

bool cfl_eval(cfl_node* node,
              cfl_definition_list* definitions,
              bool multithread);

bool cfl_evaluate_program(cfl_program* program, bool multithread);

#endif
