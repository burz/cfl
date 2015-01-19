#include "cfl_ast.h"
#include "cfl_malloc.h"

#include <stdio.h>
#include <string.h>

static bool cfl_ast_error;

void cfl_reset_ast_error_flag(void)
{
    cfl_ast_error = false;
}

bool cfl_get_ast_error_flag(void)
{
    return cfl_ast_error;
}

void* cfl_ast_malloc(size_t size)
{
    void* result = cfl_malloc(size);

    if(!result)
    {
        fprintf(stderr, "MEMORY ERROR: Ran out of memory while "
                        "creating an AST node\n");

        cfl_ast_error = true;
    }

    return result;
}
