#include "cfl_type.h"

#include <stdio.h>

int cfl_type_error;

void* cfl_type_malloc(size_t size)
{
    void* result = malloc(size);

    if(!result)
    {
        fprintf(stderr, "MEMORY ERROR: Ran out of memory "
                        "while typechecking");

        cfl_type_error = 1;
    }

    return result;
}

void cfl_create_type_variable(cfl_type* node, unsigned int id)
{
    node->type = CFL_TYPE_VARIABLE;
    node->id = id;
    node->input = 0;
    node->output = 0;
}

void cfl_create_type_bool(cfl_type* node)
{
    node->type = CFL_TYPE_BOOL;
    node->id = 0;
    node->input = 0;
    node->output = 0;
}

void cfl_create_type_integer(cfl_type* node)
{
    node->type = CFL_TYPE_INTEGER;
    node->id = 0;
    node->input = 0;
    node->output = 0;
}

void cfl_create_type_list(cfl_type* node, cfl_type* content)
{
    node->type = CFL_TYPE_LIST;
    node->id = 0;
    node->input = content;
    node->output = 0;
}

void cfl_create_type_arrow(cfl_type* node, cfl_type* input, cfl_type* output)
{
    node->type = CFL_TYPE_ARROW;
    node->id = 0;
    node->input = input;
    node->output = output;
}

int cfl_compare_type(cfl_type* left, cfl_type* right)
{
    if(left->type != right->type)
        return 1;

    if(left->type == CFL_TYPE_VARIABLE)
    {
        if(left->id != right->id)
            return 1;
    }
    else if(left->type == CFL_TYPE_LIST)
        return cfl_compare_type(left->input, right->input);
    else if(left->type == CFL_TYPE_ARROW)
        if(cfl_compare_type(left->input, right->input) ||
           cfl_compare_type(left->output, right->output))
            return 1;

    return 0;
}

int cfl_copy_type(cfl_type* target, cfl_type* node)
{
    cfl_type* input;
    cfl_type* output;

    switch(node->type)
    {
        case CFL_TYPE_VARIABLE:
            cfl_create_type_variable(target, node->id);
            break;
        case CFL_TYPE_BOOL:
            cfl_create_type_bool(target);
            break;
        case CFL_TYPE_INTEGER:
            cfl_create_type_integer(target);
            break;
        case CFL_TYPE_LIST:
            input = cfl_type_malloc(sizeof(cfl_type));

            if(!input)
                return 0;

            if(!cfl_copy_type(input, node->input))
            {
                free(input);

                return 0;
            }

            cfl_create_type_list(target, input);

            break;
        case CFL_TYPE_ARROW:
            input = cfl_type_malloc(sizeof(cfl_type));

            if(!input)
                return 0;

            output = cfl_type_malloc(sizeof(cfl_type));

            if(!output)
            {
                free(input);

                return 0;
            }

            if(!cfl_copy_type(input, node->input))
            {
                free(input);
                free(output);

                return 0;
            }

            if(!cfl_copy_type(output, node->output))
            {
                cfl_free_type(input);
                free(output);

                return 0;
            }

            cfl_create_type_arrow(target, input, output);

            break;
        default:
            break;
    }

    return 1;
}

void cfl_delete_type(cfl_type* node)
{
    if(node->input)
        cfl_free_type(node->input);

    if(node->output)
        cfl_free_type(node->output);
}

void cfl_free_type(cfl_type* node)
{
    cfl_delete_type(node);

    free(node);
}

static void cfl_print_type_inner(cfl_type* node)
{
    switch(node->type)
    {
        case CFL_TYPE_VARIABLE:
            printf("a%u", node->id);
            break;
        case CFL_TYPE_BOOL:
            printf("BOOL");
            break;
        case CFL_TYPE_INTEGER:
            printf("INTEGER");
            break;
        case CFL_TYPE_LIST:
            printf("[");
            cfl_print_type_inner(node->input);
            printf("]");
            break;
        case CFL_TYPE_ARROW:
            printf("(");
            cfl_print_type_inner(node->input);
            printf(") -> (");
            cfl_print_type_inner(node->output);
            printf(")");
            break;
        default:
            break;
    }
}

void cfl_print_type(cfl_type* node)
{
    cfl_print_type_inner(node);

    printf("\n");
}
