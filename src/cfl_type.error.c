#include "cfl_type.h"

#include <stdio.h>

bool cfl_type_error;

void* cfl_type_malloc(size_t size)
{
    void* result = malloc(size);

    if(!result)
    {   
        fprintf(stderr, "MEMORY ERROR: Ran out of memory "
                        "while typechecking");

        cfl_type_error = true;
    }   

    return result;
}

void cfl_reset_type_error_flag(void)
{
    cfl_type_error = false;
}

bool cfl_get_type_error_flag(void)
{
    return cfl_type_error;
}

void cfl_type_error_undefined_variable(char* name)
{
    if(cfl_type_error)
        return;

    fprintf(stderr, "TYPE ERROR: The variable \"%s\" must be "
                    "defined before it is used\n", name);

    cfl_type_error = true;
}

void cfl_type_error_bad_definition(char* name)
{
    if(cfl_type_error)
        return;

    fprintf(stderr, "TYPE ERROR: Could not type the definition "
                    "of \"%s\"\n", name);

    cfl_type_error = true;
}

void cfl_type_error_failure(void)
{
    if(cfl_type_error)
        return;

    fprintf(stderr, "TYPE ERROR: Could not type \"main\"\n");

    cfl_type_error = true;
}
