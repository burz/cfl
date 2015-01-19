#include "cfl_malloc.h"

#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t cfl_memory_mutex;

bool cfl_initialize_malloc(void)
{
    if(pthread_mutex_init(&cfl_memory_mutex, 0))
    {
        fprintf(stderr, "ERROR: Could not create memory mutex\n");

        return false;
    }

    return true;
}

void cfl_cleanup_malloc(void)
{
    pthread_mutex_destroy(&cfl_memory_mutex);
}

void* cfl_malloc(size_t size)
{
    pthread_mutex_lock(&cfl_memory_mutex);

    void* result = malloc(size);

    pthread_mutex_unlock(&cfl_memory_mutex);

    return result;
}

void cfl_free(void* pointer)
{
    pthread_mutex_lock(&cfl_memory_mutex);

    free(pointer);

    pthread_mutex_unlock(&cfl_memory_mutex);
}
