#include "cfl_malloc.h"

#include <stdlib>

bool cfl_initialize_malloc(void)
{
    return true;
}

void cfl_cleanup_malloc(void)
{
}

void* cfl_malloc(size_t size)
{
    return malloc(size);
}

void cfl_free(void* pointer)
{
    free(pointer);
}
