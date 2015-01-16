#include "cfl_type.h"

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
