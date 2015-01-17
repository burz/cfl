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
        const unsigned int equation_hash_table_length)
{
    equations->hash_table = cfl_type_malloc(sizeof(cfl_type_outer_hash_element*) *
                                            equation_hash_table_length);

    if(!equations->hash_table)
        return false;

    equations->equation_hash_table_length = equation_hash_table_length;

    int i = 0;
    for( ; i < equation_hash_table_length; ++i)
        equations->hash_table[i] = 0;

    return true;
}

void cfl_delete_type_equations(cfl_type_equations* equations)
{
    int i = 0, j;
    for( ; i < equations->equation_hash_table_length; ++i)
        while(equations->hash_table[i])
        {
            cfl_type_outer_hash_element* outer_pos = equations->hash_table[i];

            equations->hash_table[i] = outer_pos->next;

            cfl_free_type(outer_pos->type);

            for(j = 0; j < equations->equation_hash_table_length; ++j)
                while(outer_pos->hash_table[j])
                {
                    cfl_type_hash_element* inner_pos = outer_pos->hash_table[j];

                    outer_pos->hash_table[j] = inner_pos->next;

                    cfl_free_type(inner_pos->type);

                    free(inner_pos);
                }

            free(outer_pos->hash_table);
            free(outer_pos);
        }

    free(equations->hash_table);
}

int cfl_add_type_equation(
        cfl_type_equations* equations,
        cfl_type* left,
        cfl_type* right)
{
    unsigned long long left_hash = cfl_hash_type(left) %
                                   equations->equation_hash_table_length;

    unsigned long long right_hash = cfl_hash_type(right) %
                                    equations->equation_hash_table_length;

    return 0;
}
