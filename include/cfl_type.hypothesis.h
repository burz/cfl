#ifndef _CFL_TYPE_HYPOTHESIS_H_
#define _CFL_TYPE_HYPOTHESIS_H_

#include "cfl_type.types.h"

#include <stdbool.h>

bool cfl_push_hypothesis(cfl_type_hypothesis_chain* hypothesis_head,
                         char* name,
                         unsigned int id);

void cfl_pop_hypothesis(cfl_type_hypothesis_chain* hypothesis_head);

bool cfl_load_global_hypotheses(cfl_type_hypothesis_chain* hypothesis_head);

#endif
