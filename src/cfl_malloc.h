#ifndef _CFL_MALLOC_H_
#define _CFL_MALLOC_H_

#include <stdbool.h>
#include <stdlib.h>

bool cfl_initialize_malloc(void);
void cfl_cleanup_malloc(void);

void* cfl_malloc(size_t size);
void cfl_free(void* pointer);

#endif
