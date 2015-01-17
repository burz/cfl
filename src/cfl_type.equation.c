#include "cfl_type.h"

extern void* cfl_type_malloc(size_t size);

unsigned long long cfl_hash_type(cfl_type* type)
{
    if(type->type == CFL_TYPE_VARIABLE)
        return type->id;
    else if(type->type == CFL_TYPE_BOOL)
        return 2;
    else if(type->type == CFL_TYPE_INTEGER)
        return 3;
    else if(type->type == CFL_TYPE_CHAR)
        return 5;
    else if(type->type == CFL_TYPE_LIST)
        return 7 * cfl_hash_type(type->input);
    else if(type->type == CFL_TYPE_TUPLE)
    {
        unsigned long long sum = 0;
        int i = 0;
        for( ; i < type->id; ++i)
            sum += cfl_hash_type(((cfl_type**) type->input)[i]);
        return 11 * sum;
    }
    else if(type->type == CFL_TYPE_ARROW)
        return 13 * cfl_hash_type(type->input) *
                    cfl_hash_type(type->output);
    else
        return 0;
}

bool cfl_create_type_equations(
        cfl_type_equations* equations,
        const unsigned int equation_hash_set_length)
{
    equations->hash_set = cfl_type_malloc(sizeof(cfl_type_equation_chain) *
                                          equation_hash_set_length);

    if(!equations->hash_set)
        return false;

    equations->equation_hash_set_length = equation_hash_set_length;

    int i = 0;
    for( ; i < equation_hash_set_length; ++i)
        equations->hash_set[i].next = 0;

    return true;
}

void cfl_delete_type_equations(cfl_type_equations* equations)
{
    int i = 0;
    for( ; i < equations->equation_hash_set_length; ++i)
    {
        while(equations->hash_set[i].next)
        {
            cfl_type_hash_element* pos = equations->hash_set[i].next;

            equations->hash_set[i].next = pos->next;

            cfl_free_type(pos->type);

            while(pos->set.next)
            {
                cfl_type_set_element* set_pos = pos->set.next;

                pos->set.next = set_pos->next;

                cfl_free_type(set_pos->type);
                free(set_pos);
            }

            free(pos);
        }
    }

    free(equations->hash_set);
}
