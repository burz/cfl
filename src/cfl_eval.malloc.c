#include <stdlib.h>

#ifdef __posix

void* cfl_eval_malloc(size_t size)
{
    void* result = malloc(size);

    if(!result)
    {   
        fprintf(stderr, "MEMORY ERROR: Ran out of memory "
                        "during evaluation\n");
    }   

    return result;
}

#else

void* cfl_eval_malloc(size_t size)
{
    void* result = malloc(size);

    if(!result)
    {   
        fprintf(stderr, "MEMORY ERROR: Ran out of memory "
                        "during evaluation\n");
    }   

    return result;
}

#endif
