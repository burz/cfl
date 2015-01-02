#ifndef _CFL_EVAL_H_
#define _CFL_EVAL_H_

#include "cfl_ast.h"

int cfl_substitute(cfl_node* target, char* variable, cfl_node* value);

int cfl_evaluate(cfl_node* node);

#endif
