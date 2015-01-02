#include "cfl_typechecker.h"

#include <stdlib.h>
#include <string.h>

int cfl_create_type_variable(cfl_type* node, char* name)
{
    node->type = CFL_TYPE_VARIABLE;
    node->name = malloc(sizeof(char) * MAX_IDENTIFIER_LENGTH);

    if(!name)
        return 0;

    strcpy(node->name, name);

    node->input = 0;
    node->output = 0;

    return 1;
}

int cfl_create_type_bool(cfl_type* node)
{
    node->type = CFL_TYPE_BOOL;
    node->name = 0;
    node->input = 0;
    node->output = 0;

    return 1;
}

int cfl_create_type_arrow(cfl_type* node, cfl_type* input, cfl_type* output)
{
    node->type = CFL_TYPE_ARROW;
    node->name = 0;
    node->input = input;
    node->output = output;

    return 1;
}

int cfl_type_compare(cfl_type* left, cfl_type* right)
{
    if(left->type != right->type)
        return 1;

    if(left->type == CFL_TYPE_VARIABLE)
    {
        if(strcmp(left->name, right->name))
            return 1;
    }
    else if(left->type == CFL_TYPE_ARROW)
        if(cfl_type_compare(left->input, right->input) ||
           cfl_type_compare(left->output, right->output))
            return 1;

    return 0;
}

void cfl_delete_type(cfl_type* node)
{
    if(node->name)
        free(node->name);

    if(node->input)
    {
        cfl_delete_type(node->input);
        free(node->input);
        cfl_delete_type(node->output);
        free(node->output);
    }
}
