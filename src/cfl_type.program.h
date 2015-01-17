#ifndef _CFL_TYPE_PROGRAM_H_
#define _CFL_TYPE_PROGRAM_H_

#include "cfl_type.types.h"

unsigned int cfl_generate_global_hypotheses(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypothesis_head);

unsigned int cfl_setup_definitions(cfl_type_equations* equations,
                                   cfl_type_hypothesis_chain* hypothesis_head,
                                   cfl_definition_list* definitions);

#endif
