#include "cfl_eval.h"

bool cfl_eval(
        cfl_node* node,
        cfl_definition_list* definitions,
        bool multithread)
{
    return cfl_evaluate(node, definitions);
}